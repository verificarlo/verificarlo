package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.DialogSettings;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;

/**
 * Action to delete the saved settings containing the path to the source code if needed
 * @author Damien Thenot
 */
public class RemoveCodeFilePathAction extends Action{
	private ITmfTrace fTrace;
	
	public RemoveCodeFilePathAction(VeritraceGlobalView view, ITmfTrace trace) {
		fTrace = trace;
	}
	
	@Override
	public void run() {
		DialogSettings settings = (DialogSettings) Activator.getDefault().getDialogSettings(); //We get the DialogSettings of the plugin
		DialogSettings section = (DialogSettings) DialogSettings.getOrCreateSection(settings, "filepath"); //We get the section of settings containing the filepath

		section.removeSection(fTrace.getName()); //Delete the section containing the path, if you saved anything else in the "filepath" section it will be deleted too
	}
	
	@Override
		public String getText() {
			return "Delete saved filepath";
		}

}
