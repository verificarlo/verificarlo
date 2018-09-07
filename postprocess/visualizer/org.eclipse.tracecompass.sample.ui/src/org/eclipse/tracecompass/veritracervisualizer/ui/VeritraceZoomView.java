package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.DialogSettings;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.tracecompass.internal.tmf.ui.Activator;
import org.eclipse.tracecompass.internal.tmf.ui.ITmfImageConstants;
import org.eclipse.tracecompass.tmf.core.signal.TmfSelectionRangeUpdatedSignal;
import org.eclipse.tracecompass.tmf.core.signal.TmfSignalHandler;
import org.eclipse.tracecompass.tmf.core.signal.TmfTraceSelectedSignal;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.ui.views.TmfView;
import org.eclipse.ui.IMemento;

/**
 * Class defining the ZoomView and allowing to see the different values of a variable through time.
 * @author Damien Thenot
 */
@SuppressWarnings("restriction")
public class VeritraceZoomView extends TmfView{
	public static final String VIEW_ID = "org.eclipse.tracecompass.veritracervisualizer.ui.veritracezoomview"; //The ID of this view in the plugin

	private ITmfTrace fTrace; // The current trace of the view
	private Composite fParent; //The parent window

	private long fStartTime; //The start time getting displayed
	private long fEndTime; //The endtime getting displayed
	
	private Map<Integer, VeritraceZoomViewChart> charts = new HashMap<>(); //The charts, the key being the variable quark
	
	private Set<String> fieldToDisplay = new HashSet<>(); //The fields of the variable getting displayed in the charts
	
	private static final String fields[] = new String[] {
			"significant_digits",
			"max",
			"min",
			"std",
			"median",
			"mean"
	}; //An array of every fields contained in a variable, the field name in displayable form and it's color on the chart is defined in VeritraceZoomChart

	public long getStartTime() {
		return fStartTime;
	}

	public void setStartTime(long fStartTime) {
		this.fStartTime = fStartTime;
	}

	public long getEndTime() {
		return fEndTime;
	}

	public void setEndTime(long fEndTime) {
		this.fEndTime = fEndTime;
	}

	public VeritraceZoomView() {
		super(VIEW_ID);
		
		//react correctly if created after a trace as already been loaded
		ITmfTrace trace = getActiveTrace();
		if (trace != null) {
			traceSelected(new TmfTraceSelectedSignal(this, trace));
		}
	}

	@Override
	public void createPartControl(Composite parent) {
		fParent = parent;
		fieldToDisplay.add(VeritraceZoomViewChart.getOriginalField()); //To have one field already selected to show

		createToolbar();
	}

	/**
	 * Create the toolbar buttons
	 */
	private void createToolbar() {
		IToolBarManager toolBarManager = getViewSite().getActionBars().getToolBarManager(); 

		Menu menuBar = new Menu(fParent.getShell()); //Menu for the fields selection
		for(String act : fields) { //Create a button for every showable field
			MenuItem item = new MenuItem(menuBar, SWT.CHECK);
			if(act.equals(VeritraceZoomViewChart.getOriginalField())) { //The original selected fields need to be shown as selected at first
				item.setSelection(true);
			}
			item.setText(VeritraceZoomViewChart.getfieldsClean().get(act));
			item.addListener(SWT.Selection, new Listener() {
				public void handleEvent(Event event) {
					if(fieldToDisplay.contains(act)) { //If we already were displaying this field
						fieldToDisplay.remove(act); //we delete it from the set of field to display
						removeAllChartSeries(act); //we delete all series of this particular field on every chart
						removeAllChartAxis(act); // And we delete the axis of this field on every chart
					}
					else {
						fieldToDisplay.add(act); //We add the field to the set of displayed fields
					}
					repopulateCharts(); //We get to redo the charts
				}
			});
		}

		toolBarManager.add(new Action() { //Button fields
			@Override
			public void run() {
				menuBar.setVisible(true);
			}

			@Override
			public String getText() {
				return "Fields";
			}
		});


		toolBarManager.add(new Separator());
		toolBarManager.add(new Action() { //Reset time button
			@Override
			public void run() {
//				setStartTime(fTrace.getStartTime().toNanos());
//				setEndTime(fTrace.getEndTime().toNanos());
//				repopulateCharts();
				for(Integer key : charts.keySet()) {
					VeritraceZoomViewChart chart = charts.get(key);
					setStartTime(fTrace.getStartTime().toNanos());
					setEndTime(fTrace.getEndTime().toNanos());
					chart.selectionRange(fTrace.getStartTime().toNanos(), fTrace.getEndTime().toNanos());
				}
			}

			@Override
			public String getText() {
				return "Reset time";
			}

			@Override
			public ImageDescriptor getImageDescriptor() {
				return Activator.getDefault().getImageDescripterFromPath(ITmfImageConstants.IMG_UI_HOME_MENU); 
			}
		});
	}

	/**
	 * Destroy every axis act from all charts
	 * @param act the field of the axis to delete
	 */
	private void removeAllChartAxis(String act) {
		for(Integer key : charts.keySet()) {
			VeritraceZoomViewChart chart = charts.get(key);
			chart.removeAxis(act);
		}
	}
	
	
	/**
	 * Destroy the series of the field seriesName from every chart
	 * @param seriesName the name of the field and series to destroy
	 */
	private void removeAllChartSeries(String seriesName) {
		for(Integer act : charts.keySet()) {
			charts.get(act).removeSeries(seriesName);
		}
	}

	/**
	 * @return The current trace of the view
	 */
	protected ITmfTrace getActiveTrace() {
		return fTrace;
	}


	//	@TmfSignalHandler
	//	public void filterApplied(TmfEventFilterAppliedSignal signal) {
	//		//We draw the trace with the Filter
	//		if(fTrace == signal.getTrace()) {
	//			currentFilter = signal.getEventFilter();
	//			//			if(currentFilter != null) {
	//			//				System.out.println("Debug filterApplied : " + currentFilter.toString());
	//			//			}
	//			//			else {
	//			//				System.out.println("Debug filterApplied : currentFilter == null");
	//			//			}
	//			fTrace = null;
	//			traceSelected(new TmfTraceSelectedSignal(this, signal.getTrace()));
	//		}
	//	}
	
	
	
	/**
	 * Create a chart for a variable and configure everything needed
	 * @param variableName the name of the variable 
	 * @param variableQuark the identifier of the variable in the state system, we use it as a unique identifier
	 * @return the newly created and configured chart
	 */
	private VeritraceZoomViewChart createChart(String variableName, int variableQuark) {
		VeritraceZoomViewChart chart = new VeritraceZoomViewChart(fParent, this, getActiveTrace(), variableName, variableQuark, fieldToDisplay);
		return chart;
	}
	
	private int retry = 0; //Number of times we should retry setting the start and end time
	/**
	 * Add a chart for a variable selected from another view to the ZoomView. The variable is identified by it's quark in the StateSystem.
	 * The signal also contains the name of the variable to be able to print without crosslinking it with it's context in the state system.
	 * @param signal TmfVariableSelectedSignal
	 */
	@TmfSignalHandler
	public void variableSelected(final VariableSelectedSignal signal) {
		if(signal.getTrace() == getActiveTrace()) { //If the variable being added to the ZoomView is part of the current active trace
			int variableQuark = signal.getVariableQuark();
			if(charts.containsKey(variableQuark)) { //If the chart already exist, it is destroyed
				VeritraceZoomViewChart chart = charts.get(variableQuark); //We get the chart representing this variable
				chart.dispose();
				charts.remove(variableQuark);
				resizeCharts(); //And tell to make the chart take the size needed to look correct
			}
			else { //else it is created and populated
				VeritraceZoomViewChart chart = createChart(signal.getVariableName(), variableQuark); //We create the chart for this variable and configure everything needed
				charts.put(variableQuark, chart); //We store the chart for later uses

				while(retry > 0) { //Workaround for a bug where the chart would not show the first time when coming from another chart
					setStartTime(fTrace.getStartTime().toNanos());
					setEndTime(fTrace.getEndTime().toNanos());
					retry--;
				}
				
				chart.setTime(getStartTime(), getEndTime()); //We tell the chart the current timerange
				chart.populateChart(); //We ask the chart to load data
				chart.redraw(); //and redraw it
			}
		}
	}

	/**
	 * Tell the charts they need to change size. It is automatically done when adding a chart but need to be manually done when deleting one.
	 */
	private void resizeCharts() {
			fParent.layout(); 
	}

	/**
	 * Reload the data for every charts
	 */
	protected void repopulateCharts(){
		for(Integer act : charts.keySet()) {
			VeritraceZoomViewChart chart = charts.get(act);
			chart.setTime(getStartTime(), getEndTime());
			chart.populateChart();
		}
	}


	/**
	 * Action to perform when a new trace is selected to be displayed by the user
	 * @param signal
	 */
	@TmfSignalHandler
	public void traceSelected(final TmfTraceSelectedSignal signal) {
		// Don't populate the view again if we're already showing this trace
		if (fTrace == signal.getTrace()) {
			return;
		}
		
		if(fTrace != null) {//If a trace is already being displayed on the Zoom
			saveChartInfo();//We save what is currently being shown on the View for this trace to be able to load these information later
		}
		disposeCharts(); //Destroy every charts belonging to another trace
		
		fTrace = signal.getTrace(); //We get the new trace
		
		retry = 1; //Retry to redraw the first chart
		
		setStartTime(fTrace.getStartTime().toNanos()); //update the start and endtime for the new trace
		setEndTime(fTrace.getEndTime().toNanos());
		loadChartInfo(); //We load information about what variable to show for this trace
	}
	
	
	/**
	 * Called when the workbench is closed, so when eclipse is closed
	 */
	@Override
	public void saveState(IMemento memento) {
		super.saveState(memento);
		
		DialogSettings settings = (DialogSettings) Activator.getDefault().getDialogSettings();
		settings.removeSection(VeritraceZoomView.VIEW_ID); //We delete the information on the current ZoomView
	}
	
	/**
	 * Load information about the state of the view on a specific trace
	 */
	private void loadChartInfo() {
		DialogSettings settings = (DialogSettings) Activator.getDefault().getDialogSettings(); //To obtain the DialogSettings of the plugin
		DialogSettings section = (DialogSettings) DialogSettings.getOrCreateSection(settings, VeritraceZoomView.VIEW_ID); //We save the information in a section for this view 
		DialogSettings sectionLoc = (DialogSettings) section.getSection(fTrace.getName());
		
		if(sectionLoc != null) { //If we have information else we don't have anything to do
			String idVariableArray[] = sectionLoc.getArray("id"); //We get the array of quark of the variable we will display chart for

			for(String idVariable : idVariableArray) { 
				String nameVariable = sectionLoc.get(idVariable);//We get the name if this variable to be able to display it
				broadcast(new VariableSelectedSignal(this, getActiveTrace(), nameVariable, Integer.parseInt(idVariable))); //We create a chart for every variable needing one
			}
		}
		
	}
	
	/**
	 * Save information on a trace for later uses
	 */
	private void saveChartInfo() {
			DialogSettings settings = (DialogSettings) Activator.getDefault().getDialogSettings(); //To obtain the DialogSettings of the plugin
			DialogSettings section = (DialogSettings) DialogSettings.getOrCreateSection(settings, VeritraceZoomView.VIEW_ID); //We save the information in a section for this view
			section.removeSection(fTrace.getName()); //We delete previously saved information about this trace if they exist 
			DialogSettings sectionLoc = (DialogSettings) DialogSettings.getOrCreateSection(section, fTrace.getName()); //We create or recreate a section for the trace to save our information about the trace
			
			List<Integer> idVariableList = new ArrayList<>();
			for(Integer idVariable : charts.keySet()) { //We save the quark of the state system of every variable currently shown
					idVariableList.add(idVariable);
			}
			
			String idVariableArray[] = new String[idVariableList.size()]; //To be able to save in a DialogSettings we need an array of String
			for(int i = 0; i<idVariableList.size(); i++) {
				int idVariable = idVariableList.get(i);
				String idVariableString = Integer.toString(idVariable);
				idVariableArray[i] = idVariableString;
				sectionLoc.put(idVariableString, charts.get(idVariable).getVariableName()); //We save the name of the variable with the key being the quark to be able to retrieve it later.
			}
			
			sectionLoc.put("id", idVariableArray); //We save the newly created array of quark
	}
	
	/**
	 * Update the shown timerange on the charts and launch the reloading of the date on the new timerange.
	 * @param signal the {@link TmfSelectionRangeUpdatedSignal} received
	 */
	@TmfSignalHandler
	public void rangeApplied(TmfSelectionRangeUpdatedSignal signal) {
		if(fTrace == signal.getTrace()) {
			if(signal.getBeginTime().compareTo(signal.getEndTime()) < 0) { //The order of the selected timerange need to be verified and put in the start < end order
				setStartTime(signal.getBeginTime().toNanos());
				setEndTime(signal.getEndTime().toNanos());
			}
			else {
				setStartTime(signal.getEndTime().toNanos());
				setEndTime(signal.getBeginTime().toNanos());
			}
			
			for(Integer key : charts.keySet()) { //For every chart, we tell it the selectionRange has been updated
				charts.get(key).selectionRange(getStartTime(), getEndTime());
			}
		}
	}
	
	/**
	 * Destroy every charts
	 */
	private void disposeCharts() {
		for(Integer act : charts.keySet()) {
			charts.get(act).dispose();
		}
		charts.clear();
	}

	@Override
	public void setFocus() {
		fParent.setFocus();
		for(Integer key : charts.keySet()) {
			charts.get(key).setFocus();
		}
		fParent.setFocus();
	}
}