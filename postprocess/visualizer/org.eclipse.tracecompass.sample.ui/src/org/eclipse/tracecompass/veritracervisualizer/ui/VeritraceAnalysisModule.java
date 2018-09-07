package org.eclipse.tracecompass.veritracervisualizer.ui;

import static org.eclipse.tracecompass.common.core.NonNullUtils.checkNotNull;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.eclipse.tracecompass.statesystem.core.ITmfStateSystem;
import org.eclipse.tracecompass.statesystem.core.exceptions.AttributeNotFoundException;
import org.eclipse.tracecompass.statesystem.core.exceptions.StateSystemDisposedException;
import org.eclipse.tracecompass.statesystem.core.exceptions.StateValueTypeException;
import org.eclipse.tracecompass.statesystem.core.exceptions.TimeRangeException;
import org.eclipse.tracecompass.statesystem.core.interval.ITmfStateInterval;
import org.eclipse.tracecompass.statesystem.core.statevalue.ITmfStateValue;
import org.eclipse.tracecompass.tmf.core.analysis.requirements.TmfAbstractAnalysisRequirement;
import org.eclipse.tracecompass.tmf.core.analysis.requirements.TmfAbstractAnalysisRequirement.PriorityLevel;
import org.eclipse.tracecompass.tmf.core.analysis.requirements.TmfAnalysisEventFieldRequirement;
import org.eclipse.tracecompass.tmf.core.analysis.requirements.TmfAnalysisEventRequirement;
import org.eclipse.tracecompass.tmf.core.statesystem.ITmfStateProvider;
import org.eclipse.tracecompass.tmf.core.statesystem.TmfStateSystemAnalysisModule;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;

import com.google.common.collect.ImmutableSet;

/**
 * The analysis module used to extract information about a Veritrace trace and show the results into the views.
 * @author Damien Thenot
 *
 */
public class VeritraceAnalysisModule extends TmfStateSystemAnalysisModule {
	public static final String ID = "org.eclipse.tracecompass.veritracervisualizer.ui.veritraceanalysismodule"; //ID used to define the module in the plugin
//	public static final Integer TOTAL = -1;

	public VeritraceAnalysisModule() {
		super();
	}

	@Override
	protected ITmfStateProvider createStateProvider() {
		ITmfTrace trace = checkNotNull(getTrace());
		return new VeritraceStateProvider(trace);
	}

	@Override
	protected StateSystemBackendType getBackendType() {
		return StateSystemBackendType.FULL;
	}

	@Override
	public Iterable<TmfAbstractAnalysisRequirement> getAnalysisRequirements() {
		Set<String> requiredEvents = ImmutableSet.of(Value.EventName, Context.EventName);
		TmfAbstractAnalysisRequirement eventsReq1 = new TmfAnalysisEventRequirement(requiredEvents, PriorityLevel.MANDATORY);

		Set<String> requiredEventFields = ImmutableSet.of("context", "significant_digits", "parent");
		TmfAbstractAnalysisRequirement eventFieldRequirement = new TmfAnalysisEventFieldRequirement(
				Value.EventName,
				requiredEventFields,
				PriorityLevel.MANDATORY);

		Set<String> requiredEventFields2 = ImmutableSet.of("name", "id");
		TmfAbstractAnalysisRequirement eventFieldRequirement2 = new TmfAnalysisEventFieldRequirement(
				Context.EventName,
				requiredEventFields2,
				PriorityLevel.MANDATORY);

		Set<TmfAbstractAnalysisRequirement> requirements = ImmutableSet.of(eventsReq1, eventFieldRequirement, eventFieldRequirement2);
		return requirements;
	}

	/**
	 * Allow to extract from the State System a value of field for every variable.
	 * The keys of the returned map are the quark in the State System of the variable.
	 * @param startParam
	 * @param endParam
	 * @param field
	 * @return a Map of the value of every variable of field from the given start timestamp, to the last
	 */
	public Map<Integer, Double> getValuesRange(long startParam, long endParam, String field) {
		ITmfTrace trace = getTrace(); //we get the trace
		if (trace == null) { //and check it exist
			return Collections.<Integer, Double> emptyMap();
		}
		
		ITmfStateSystem ss = TmfStateSystemAnalysisModule.getStateSystem(trace, VeritraceAnalysisModule.ID); //We obtain the state system linked to that trace throught the AnalysisModule
		//It allow us to be sure it is the correct one
		if (ss == null) { //And of course, we check if it exist
			return Collections.<Integer, Double> emptyMap();
		}

		long start = Math.max(startParam, ss.getStartTime()); //We check the start and end timestamp given by the user exists in the trace
		long end = Math.min(endParam, ss.getCurrentEndTime());
		
		/*
		 * Make sure the start/end times are within the state history, so we
		 * don't get TimeRange exceptions.
		 */
		long startTime = ss.getStartTime();
		long endTime = ss.getCurrentEndTime();
		if (endTime < startTime) {
			return Collections.<Integer, Double> emptyMap();
		}

		Map<Integer, Double> map = new HashMap<>();
		try {
			int variableNode = ss.getQuarkAbsolute(Value.EventRoot);//We get the root of the values in the StateSystem
			List<Integer> variableQuarks = ss.getSubAttributes(variableNode, false); // We obtain the list of every variable in the state system
			
			/* Query full states at start and end times */
			List<ITmfStateInterval> startState = ss.queryFullState(start);
			List<ITmfStateInterval> endState = ss.queryFullState(end);
			for (Integer variableQuark : variableQuarks) { 
				int quarkValue = ss.getQuarkRelative(variableQuark, field); //We look the field of every variable
				ITmfStateValue startStatevalue = startState.get(quarkValue).getStateValue(); // at the beginning of the range
				//TODO: startState.getStartTime()
				ITmfStateValue endStatevalue = endState.get(quarkValue).getStateValue(); // And at the end
				Double startValue = Double.MAX_VALUE;
				Double endValue = Double.MAX_VALUE;
				
				if(!(startStatevalue.isNull())){ //If the value exist, we extract the value
					startValue = startStatevalue.unboxDouble();
				}
				if(!(endStatevalue.isNull())) {
					endValue = endStatevalue.unboxDouble();
				}
				if(!(startStatevalue.isNull()) || !(endStatevalue.isNull())) {
					map.put(variableQuark, Double.min(startValue, endValue)); //And put the minimum of it in the map
					//If the range is to big, interesting data in the middle of the range would not be extracted.
					//The range need to be small enough to avoid that. But if we go 1 by 1, it could cost a lot for big timerange
				}
			}
		}
		catch (TimeRangeException | AttributeNotFoundException e) {
			e.printStackTrace();
		} 
		catch (StateValueTypeException | StateSystemDisposedException e) {
			e.printStackTrace();
		}

		return map;

	}
}
