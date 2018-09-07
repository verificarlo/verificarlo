package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.Collection;
import java.util.Map;
import java.util.Set;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.tracecompass.tmf.core.analysis.IAnalysisModule;
import org.eclipse.tracecompass.tmf.core.event.ITmfEvent;
import org.eclipse.tracecompass.tmf.core.event.aspect.ITmfEventAspect;
import org.eclipse.tracecompass.tmf.core.event.aspect.TmfBaseAspects;
import org.eclipse.tracecompass.tmf.core.exceptions.TmfTraceException;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.core.trace.TraceValidationStatus;
import org.eclipse.tracecompass.tmf.ctf.core.event.CtfTmfEventType;
import org.eclipse.tracecompass.tmf.ctf.core.trace.CtfTmfTrace;
import org.eclipse.tracecompass.tmf.ctf.core.trace.CtfTraceValidationStatus;

import com.google.common.collect.ImmutableList;

/**
 * Class representing the VeriTracer trace type. 
 * It validate the trace type, define the aspects to show in the Event View and allow to obtain the list of analysis module working on that trace type.
 * @author Damien Thenot
 *
 */
public class Veritrace extends CtfTmfTrace implements ITmfTrace{
		
	/**
	 * The list of aspects, it defines the column which will appear in the Event View and how to obtain data to show in these columns. 
	 */
	public static final Collection<ITmfEventAspect<?>> VERITRACE_ASPECTS = 
            ImmutableList.of(
//                    VeritraceAspects.getTimestampAspect(),
            		TmfBaseAspects.getTimestampAspect(),
                    TmfBaseAspects.getEventTypeAspect(),
                    VeritraceAspects.getVariableNameAspect(),
                    VeritraceAspects.getVariableTypeAspect(),
                    VeritraceAspects.getVariableFileAspect(),
                    VeritraceAspects.getVariableLineAspect(),
                    VeritraceAspects.getContentAspect("max", "Max", "Maximum"), //the true name of the fields, the name shown in the column and the help text
                    VeritraceAspects.getContentAspect("min", "Min", "Minimum"),
                    VeritraceAspects.getContentAspect("median", "Median", "Median"),
                    VeritraceAspects.getContentAspect("mean", "Mean", "Mean"),
                    VeritraceAspects.getContentAspect("std", "Standard Deviation", "Standard deviation"),
                    VeritraceAspects.getContentAspect("significant_digits", "Significants digits", "Significant number digits"),
//                  TmfBaseAspects.getContentsAspect()
                    VeritraceAspects.getContentsAspect() //Column showing all the content of an event, is not visible by default
                    );
	
	public Veritrace() {
		super();
	}
	
	/**
	 * Define a test to see if the trace is really of Veritrace type.
	 */
	@Override
	public IStatus validate(IProject project, String path) {
		IStatus status = super.validate(project, path);
		if(status instanceof CtfTraceValidationStatus) {
			Map<String, String> environment = ((CtfTraceValidationStatus) status).getEnvironment();
            String domain = environment.get("domain"); //$NON-NLS-1$ 
            if (domain == null || !domain.equals("\"veritrace\"")) { //$NON-NLS-1$ 
                return new Status(IStatus.ERROR, Activator.PLUGIN_ID, "This trace isn't a compatible Veritrace trace."); 
            } 
            return new TraceValidationStatus(100, Activator.PLUGIN_ID); 
        }
        return status; 
	}
	
	
	@Override
    public void initTrace(IResource resource, String path,
            Class<? extends ITmfEvent> eventType) throws TmfTraceException {
        super.initTrace(resource, path, eventType);
	}
	
	@Override
    public Set<CtfTmfEventType> getContainedEventTypes() {
        return super.getContainedEventTypes();
	}
	
	/**
	 * Return the aspects' list for this trace type.
	 * It is used by the Event View to data in the columns.
	 */
	@Override
    public Iterable<ITmfEventAspect<?>> getEventAspects() {
        return VERITRACE_ASPECTS;
    }
	
	/**
	 * Gives an iterable list of the analysis module existing for that trace type.
	 */
	@Override
	public Iterable<IAnalysisModule> getAnalysisModules() {
		Iterable<IAnalysisModule> analysisModule = super.getAnalysisModules();
		return analysisModule;
	}
}
