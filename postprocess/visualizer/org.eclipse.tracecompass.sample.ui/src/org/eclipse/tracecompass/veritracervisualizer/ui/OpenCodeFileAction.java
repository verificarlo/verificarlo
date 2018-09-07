package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.io.File;

import org.eclipse.core.filesystem.EFS;
import org.eclipse.core.filesystem.IFileStore;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.DialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.text.BadLocationException;
import org.eclipse.jface.text.IDocument;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.ide.IDE;
import org.eclipse.ui.texteditor.ITextEditor;

/**
 * Action opening source code at a specific line.
 * If the path to the source code is saved it will open it else it will ask the user the path and save for later use before opening the file to the desired line
 * @author Damien Thenot
 */
public class OpenCodeFileAction extends Action {
	private VeritraceGlobalView fView; //The view requesting the action
	private String fVariableName; //The name of the variable
	private int fVariableQuark; //The quark of the variable in the State System
	private int fLine; //The line where the file needs to be highlighted
	private String fFile; //The filename (or localpath + name) to open  
	private ITmfTrace fTrace; //The trace from the source code

	public OpenCodeFileAction(VeritraceGlobalView source, String name, int variableQuark, String file, int line, ITmfTrace trace) {
		fView = source;
		fVariableName = name;
		fVariableQuark = variableQuark;
		fLine = line;
		fFile = file;
		fTrace = trace;
	}

	/**
	 * What is shown in the right-click menu on the {@link VeritraceGlobalView}.
	 */
	@Override
	public String getText() {
		return "Show " + fVariableName + " in file " + fFile + " at line " + fLine;
	}

	public String getName() {
		return fVariableName;
	}

	public int getLine() {
		return fLine;
	}

	public String getFile() {
		return fFile;
	}

	public int getQuark() {
		return fVariableQuark;
	}	

	/**
	 * Function giving the path to the source for the trace if saved else ask the user for the path and save it.
	 * @return the path to the source code of this trace
	 */
	private String obtainSourcePath() {
		DialogSettings settings = (DialogSettings) Activator.getDefault().getDialogSettings();
		DialogSettings section = (DialogSettings) DialogSettings.getOrCreateSection(settings, "filepath"); //We save the path under filepath in the setting file of our plugin
		DialogSettings sectionLoc = (DialogSettings) DialogSettings.getOrCreateSection(section, fTrace.getName()); //We have a section for the trace to save our information about the trace
		
		String path = sectionLoc.get("path"); //We try to load the path

		if(path == null) { //If it doesn't already exist
			path = getSourcePathFromUser(); //We obtain it from the user
			sectionLoc.put("path", path); //We save it with the key being the name of the trace
		}

		return path; //And return it to the caller
	}

	/**
	 * Open a dialog window to obtain the path to the source code from the user
	 * @return a String containing the user entered path
	 */
	private String getSourcePathFromUser() {
		DirectoryDialog dialog = new DirectoryDialog(fView.getParentComposite().getShell(), SWT.BORDER);
		dialog.setFilterPath("~");
		String result = dialog.open();
		return result;
	}

	/**
	 * Executed when the user click on the option.
	 * Try to obtain the path the source code and ask the filesystem handler from Eclipse to open in the correct editor for the file.
	 * And then ask the editor to go and highlight the correct line.
	 */
	@Override
	public void run() {
		if(fFile.isEmpty()) {
			return;
		}

		String path = obtainSourcePath(); //Absolute path to the root of the source file
		final String separator = System.getProperty("file.separator"); //We get the OS specific separator to use 
		File fileToOpen = new File(path + separator + fFile); //We create a new File object containing the path to the source file
		if(fileToOpen.exists() && fileToOpen.isFile()) { //We verify it exist and is a file
			IFileStore fileStore = EFS.getLocalFileSystem().getStore(fileToOpen.toURI()); //We obtain a handle for the file
			IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage(); // We obtain the current page, the eclipse window where we can open the editor

			try {
				IEditorPart editor = IDE.openEditorOnFileStore(page, fileStore); //We ask to open the correct editor on the file at the location of the filestore in the window, the page.
				if(editor instanceof ITextEditor) {
					ITextEditor textEditor = (ITextEditor) editor;
					IDocument document = textEditor.getDocumentProvider().getDocument(textEditor.getEditorInput());
					textEditor.selectAndReveal(document.getLineOffset(fLine - 1) , document.getLineLength(fLine-1)); 
					//We highlight the correct line, the offset and length of the line is calculated thanks to document
				}
			}
			catch (PartInitException | BadLocationException e) {
				e.printStackTrace();
			}
		}
		else {
//			System.out.println(path + "/" + fFile + " does not exist.");
			MessageDialog.openInformation(fView.getParentComposite().getShell(), "Source file not found", "The source file containing this variable doesn't exist in the given path.");
		}
	}

}
