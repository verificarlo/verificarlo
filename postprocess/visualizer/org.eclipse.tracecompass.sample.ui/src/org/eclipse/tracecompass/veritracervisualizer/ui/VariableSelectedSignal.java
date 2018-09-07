package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.tracecompass.tmf.core.signal.TmfSignal;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;

/**
 * Signal created when the user wants to add or remove a variable from the ZoomView
 * @author Damien Thenot
 */
public class VariableSelectedSignal extends TmfSignal {
	private int fVariableQuark; //The quark of the variable in the State System
	private String fName; //The name of the variable to add to the Zoom View
	private ITmfTrace fTrace; // The trace this variable is from
	
	public VariableSelectedSignal(Object source) {
		super(source);
	}

	public VariableSelectedSignal(Object source, int reference) {
		super(source, reference);
	}
	
	/**
	 * The signal need to specify the name and the location of the data of the variable in the state system, in the form of it's quark.
	 * @param source view where the signal is originating from
	 * @param trace the trace the variable is coming from
	 * @param name name of the variable
	 * @param variableQuark identifier of the variable in the State System
	 */
	public VariableSelectedSignal(Object source, ITmfTrace trace, String name, int variableQuark) {
		super(source);
		fVariableQuark = variableQuark;
		fName = name;
		fTrace = trace;
	}
	
	/**
	 * @return the quark of the variable, it's identifier in the State System
	 */
	public int getVariableQuark() {
		return fVariableQuark;
	}

	public String getVariableName() {
		return fName;
	}

	public ITmfTrace getTrace() {
		return fTrace;
	}
}
