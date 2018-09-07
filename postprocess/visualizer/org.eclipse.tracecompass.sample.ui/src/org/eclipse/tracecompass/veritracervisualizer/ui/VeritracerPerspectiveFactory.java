package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.tracecompass.tmf.ui.project.wizards.NewTmfProjectWizard;
import org.eclipse.ui.IFolderLayout;
import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * Define the Veritracer perspective
 * @author Damien Thenot
 *
 */
public class VeritracerPerspectiveFactory implements IPerspectiveFactory {

	private static final String ID = "org.eclipse.tracecompass.veritracervisualizer.ui.veritracerperspective"; //ID which identify it with the plugin
	
	private static final String GLOBAL_VIEW_ID = VeritraceGlobalView.VIEW_ID; //ID of the GlobalView
	private static final String ZOOM_VIEW_ID = VeritraceZoomView.VIEW_ID; //ID of the ZoomView
	private static final String PROJECT_VIEW_ID = IPageLayout.ID_PROJECT_EXPLORER; //Id of the project explorer

	public VeritracerPerspectiveFactory() {
	}

	/**
	 * Create the layout and the actions of the perspective
	 */
	@Override
	public void createInitialLayout(IPageLayout layout) {
		defineLayout(layout);
		defineActions(layout);
	}
	
	
	/**
	 * Define the location of the views in the workbench
	 * @param layout the layout
	 */
	private void defineLayout(IPageLayout layout) {
		//Project view on the left
		IFolderLayout topLeft = layout.createFolder("left", IPageLayout.LEFT, 0.15f, IPageLayout.ID_EDITOR_AREA);
		topLeft.addView(PROJECT_VIEW_ID);

		//Global view on the middle up
		IFolderLayout upLeft = layout.createFolder("upLeft", IPageLayout.TOP, 0.85f, IPageLayout.ID_EDITOR_AREA);
		upLeft.addView(GLOBAL_VIEW_ID);

		//Zoom View on the right up
		IFolderLayout upRight = layout.createFolder("upRight", IPageLayout.RIGHT, 0.50f, "upLeft");
		upRight.addView(ZOOM_VIEW_ID);
		
		//With the editor on the bottom
	}
	

	/**
	 * Define the views in the show view fast access
	 * @param layout the layout
	 */
	private void defineActions(IPageLayout layout) {
		layout.addNewWizardShortcut(NewTmfProjectWizard.ID);
		layout.addNewWizardShortcut("org.eclipse.ui.wizards.new.folder");
        layout.addNewWizardShortcut("org.eclipse.ui.wizards.new.file");
        
		// Add "show views". They will be present in "show view" menu
		layout.addShowViewShortcut(GLOBAL_VIEW_ID);
		layout.addShowViewShortcut(ZOOM_VIEW_ID);
		

		// Add  "perspective short cut"
		layout.addPerspectiveShortcut(VeritracerPerspectiveFactory.ID);
	}

}
