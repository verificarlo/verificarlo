package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.jface.action.Action;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.ui.views.TmfView;

/**
 * Action for adding a variable to the {@link VeritraceZoomView} 
 * @author Damien Thenot
 */
public class ZoomVariableAction extends Action {
	private TmfView fView; //The view having this action, used for the signal
	private String fVariableName; //name of the variable getting zoomed
	private int fVariableQuark; //quark in the State System of the variable getting zoomed
	private ITmfTrace fTrace; //The trace the variable is from
	
	/**
	 * Constructor of the action
	 * @param source the source view for this action
	 * @param variableName the name of the variable this action is supposed to zoom if run
	 * @param variableQuark the quark of the variable this action is supposed to zoom if run
	 * @param trace the trace this variable is part of
	 */
	public ZoomVariableAction(TmfView source, String variableName, int variableQuark, ITmfTrace trace) {
		fView = source;
		fVariableName = variableName;
		fVariableQuark = variableQuark;
		fTrace = trace;
	}
	
	/**
	 * Text of the button for this action.
	 */
	@Override
	public String getText() {
		return "Add/Remove to Zoom View";
	}
	
	/**
	 * Code executed if the action is run. Send a signal throught the view requesting the zoom to everyone who would listen a {@link VariableSelectedSignal}.
	 */
	@Override
	public void run() {
		fView.broadcast(new VariableSelectedSignal(fView, fTrace, fVariableName, fVariableQuark)); //Broadcast a signal through TmfSignalManager allowing everyone needing to know which variable should be zoomed on 
        super.run();
	}
}
