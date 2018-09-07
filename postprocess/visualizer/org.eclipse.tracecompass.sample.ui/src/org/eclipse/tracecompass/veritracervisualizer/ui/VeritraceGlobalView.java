package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.tracecompass.statesystem.core.ITmfStateSystem;
import org.eclipse.tracecompass.statesystem.core.exceptions.AttributeNotFoundException;
import org.eclipse.tracecompass.statesystem.core.exceptions.StateSystemDisposedException;
import org.eclipse.tracecompass.tmf.core.signal.TmfSignalHandler;
import org.eclipse.tracecompass.tmf.core.signal.TmfTraceSelectedSignal;
import org.eclipse.tracecompass.tmf.core.statesystem.TmfStateSystemAnalysisModule;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.core.trace.TmfTraceUtils;
import org.eclipse.tracecompass.tmf.ui.views.timegraph.AbstractTimeGraphView;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.TimeGraphPresentationProvider;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeEvent;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeGraphEntry;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.TimeGraphEntry;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.widgets.Utils;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.widgets.Utils.TimeFormat;
import org.eclipse.ui.IWorkbenchActionConstants;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;

/**
 * Class defining the GlobalView, it uses {@link AbstractTimeGraphView} to show informations.
 * @see org.eclipse.tracecompass.tmf.ui.views.timegraph.AbstractTimeGraphView
 * @author Damien Thenot
 */
@SuppressWarnings("unchecked")
public class VeritraceGlobalView extends AbstractTimeGraphView {

	public final static String VIEW_ID = "org.eclipse.tracecompass.veritracervisualizer.ui.veritraceglobalview"; //The ID identifying this view in the plugin

	private static final long BUILD_UPDATE_TIMEOUT = 500L; //The timeout in milliseconds between every wait for the State System building process to finish

	private static final String VARIABLE_COLUMN = "Variable"; //Name of the column showing the name of the variables
	private static final String TYPE_COLUMN = "Type"; //Name of the column showing the type of the variables
	private static final String LINE_COLUMN = "Line"; //Name of the column showing the line of the variables in their source file


	private Action fCustomFilter; //Action executed by the s< button and which hide variables and events if they are not problematic
	private boolean isCustomFilterEnabled = false;

	private Map<Integer, TimeGraphEntry> callpaths = new HashMap<Integer, TimeGraphEntry>(); //Map linking the callpath identifier with the entry in the treeview
	private Map<Long, Context> contexts = new HashMap<Long, Context>(); //Map of the context with the key being the hash of the variables allowing to link them to the correct context

	/**
	 * Define the columns of the tree view
	 */
	private static final String[] columns = new String[] {
			VARIABLE_COLUMN,
			TYPE_COLUMN,
			LINE_COLUMN
	};

	/**
	 * Define the columns of the filter dialog window
	 */
	private static final String[] filterColumns = new String[] {
			VARIABLE_COLUMN,
			TYPE_COLUMN
	};

	private static final int initialSortColumn = 0; //The tree view is alphabetically sorted at initialization
	private static final Comparator<ITimeGraphEntry>[] comparators; //Comparators allowing to sort the columns
	static {
		ImmutableList.Builder<Comparator<ITimeGraphEntry>> builder = ImmutableList.builder();
		builder.add(VeritraceGlobalViewColumnComparators.VARIABLE_NAME_COLUMN_COMPARATOR) //Here we give the comparators in the same order as the columns
		.add(VeritraceGlobalViewColumnComparators.TYPE_COLUMN_COMPARATOR)
		.add(VeritraceGlobalViewColumnComparators.LINE_COLUMN_COMPARATOR);
		List<Comparator<ITimeGraphEntry>> l = builder.build();
		comparators = l.toArray(new Comparator[l.size()]);
	}

	public VeritraceGlobalView(String id, TimeGraphPresentationProvider pres) {
		super(id, pres);
	}

	public VeritraceGlobalView() {
		super(VIEW_ID, new VeritraceGlobalViewPresentationProvider()); //We create the TimeGraph with our own presentation provider allowing us to define our own colors for events
		setTreeColumns(columns, comparators, initialSortColumn); //Columns of the tree view
		setTreeLabelProvider(new VeritraceTreeLabelProvider()); //Provider of data for the tree view by entry
		setFilterColumns(filterColumns); //columns displayed in the filter dialog window
		setFilterLabelProvider(new VeritraceTimeGraphFilterLabelProvider()); //Provider of data for the filter
	}

	@Override
	public void createPartControl(Composite parent) {
		super.createPartControl(parent);
		getTimeGraphViewer().setTimeFormat(TimeFormat.CALENDAR); //We redefine the timeformat of the viewer the way we want (here with EPOCH time)
	}

	/**
	 * This provider is used to differentiate how to obtain data for the columns in the tree view.
	 * @author Damien Thenot
	 */
	protected static class VeritraceTreeLabelProvider extends TreeLabelProvider {
		@Override
		public String getColumnText(Object element, int columnIndex) {
			if (columnIndex == 0 && element instanceof TimeGraphEntry) {
				return ((TimeGraphEntry) element).getName();
			}

			if (element instanceof VeritraceGlobalViewEntry) { //If the element is one of our one entry in the TimeGraph
				VeritraceGlobalViewEntry entry = (VeritraceGlobalViewEntry) element; 

				if(columns[columnIndex].equals(VARIABLE_COLUMN)) { //If we are on the column meant to contain variable name (or callpath (function))
					return entry.getName(); //We give the name
				}
				else if(columns[columnIndex].equals(TYPE_COLUMN)){ //else if it's the type
					return entry.getTypeName(); //We give the type name, the clean name for the type
				}
				else if(columns[columnIndex].equals(LINE_COLUMN)) { //else if it's the line
					int line = entry.getLine();
					if(line == -1 || line == 0) { //If the information does not fit being shown or if the information is not present in the trace
						return "";
					}
					else {
						return String.valueOf(entry.getLine());
					}

				}
			}
			return ""; //If we don't know what the object is or what column it is, we don't show anything
		}
	}

	/**
	 * Data provider for the filter dialog window
	 * @author Damien Thenot
	 */
	private static class VeritraceTimeGraphFilterLabelProvider extends TreeLabelProvider {
		@Override
		public String getColumnText(Object element, int columnIndex) {
			if (columnIndex == 0 && element instanceof TimeGraphEntry) {
				return ((TimeGraphEntry) element).getName();
			} else if (columnIndex == 1 && element instanceof VeritraceGlobalViewEntry) {
				//                return Integer.toString(((VeritraceGlobalViewEntry) element).getType());
				return ((VeritraceGlobalViewEntry) element).getTypeName();
			}
			return "";
		}
	}

	/**
	 * Give or create the action for the custom filter
	 * @return the action
	 */
	private IAction getCustomFilterAction() {
		if (fCustomFilter == null) {
			fCustomFilter = new CustomFilterAction(this, getParentComposite());
		}
		return fCustomFilter;
	}

	private CustomFilterInterval fFilter; //An interval to identify problematic parts of an entry
	/**
	 * Create the filter and apply it when necessary to hide unwanted entry or events on the TimeGraph
	 * @author Damien Thenot
	 */
	private class CustomFilterAction extends Action{
		private CustomFilterDialog dialog; //Dialog box where the user enter the information

		//		private VeritraceGlobalView fView;
		public CustomFilterAction(VeritraceGlobalView veritraceGlobalView, Composite composite) {
			super("s<", AS_CHECK_BOX); //the first parameter is the string printed to the user
			setToolTipText("Filter on the significant digits");
			//			fView = veritraceGlobalView;
			fFilter = new CustomFilterInterval(-Double.MAX_VALUE, Double.MAX_VALUE); //We create a filter
			dialog = new CustomFilterDialog(composite.getShell(), fFilter); //we create the dialog window
		}

		private double limit;
		private final ViewerFilter fSignificantDigitsFilter = new ViewerFilter() {
			@Override
			public boolean select(Viewer viewer, Object parentElement, Object element) {
				if(element instanceof VeritraceGlobalViewEntry) {
					VeritraceGlobalViewEntry root = (VeritraceGlobalViewEntry) element; //The root entry of the trace

					Iterable<VeritraceGlobalViewEntry> attributeEntries = Iterables.filter(Utils.flatten(root), VeritraceGlobalViewEntry.class); //We extract only VeritraceGlobalViewEntry from the flatten entries tree
					for(VeritraceGlobalViewEntry entry : attributeEntries) { //For every entry
						if(entry.isProblematic(limit)) { //We check if it needs to be shown or hidden
							return true;  //if the entry has at least one event below the limit, it is problematic and true is returned
						}
					}
				}
				return false;
			}
		};

		public void addFilter() {
			isCustomFilterEnabled = true; 
			limit = dialog.getLimit(); //and obtain the user chosen limit

			getTimeGraphViewer().addFilter(fSignificantDigitsFilter); //and add the filter to the timegraph
		}

		public void removeFilter() {
			isCustomFilterEnabled = false;
			setChecked(false);
			getTimeGraphViewer().removeFilter(fSignificantDigitsFilter); //We remove the filter from the timegraph
			List<TimeGraphEntry> entryList = getEntryList(getTrace());
			if(entryList != null && !entryList.isEmpty()) {
				VeritraceGlobalViewEntry root = (VeritraceGlobalViewEntry) entryList.get(0);
				Iterable< VeritraceGlobalViewEntry> attributeEntries = Iterables.filter(Utils.flatten(root), VeritraceGlobalViewEntry.class); //We extract only VeritraceGlobalViewEntry from the flatten entries tree
				for(VeritraceGlobalViewEntry entry : attributeEntries) { //For every entry
					Iterator<ITimeEvent> timeEventsIterator = entry.getTimeEventsIterator();
					while(timeEventsIterator.hasNext()) {							
						ITimeEvent event = timeEventsIterator.next();
						if(event instanceof VeritraceTimeEvent) {
							VeritraceTimeEvent castEvent = (VeritraceTimeEvent) event;
							castEvent.setVisibility(true); //Here we get all the events of all the entries and set their visibility to true because the filter is deactivated
						}
					}
				}
			}

		}

		@Override
		public void run() {
			super.run();
			if(!isCustomFilterEnabled) { //if push the button from it's inactive state
				if(dialog.open() == 0){ //We open the dialog window and if the user choose OK 
					addFilter();
				}
				else {//if the user choose Cancel
					removeFilter();

				}
			}
			else { //else if the user disable the filter
				removeFilter();
			}		
		}
	}

	/**
	 * Function allowing the creation of the callpath tree. You call it with the quark of the root of the callpath in the state system and it will build the entries
	 * and return the entry corresponding to the root.
	 * This method will build callpaths, a map indexed by the callpath identifier to be used to link the callpath context with the corresponding entry.
	 * @param trace
	 * @param startTime
	 * @param endTime
	 * @param ssq ITmfStateSystem
	 * @param quark the quark of the callpath statesystem entry
	 * @return the entry of the callpath
	 */
	protected  TimeGraphEntry recursiveCallpathRead( ITmfTrace trace, long startTime, long endTime, final ITmfStateSystem ssq, int quark){
		String name = ssq.getAttributeName(quark);
		int id = 0; //id is the hash of the variable which is used to link context and variable
		try {
			id = ssq.querySingleState(ssq.getCurrentEndTime(), quark).getStateValue().unboxInt(); //we get the identifier of the callpath from the state system
		} catch (StateSystemDisposedException e) {
			e.printStackTrace();
		}

		TimeGraphEntry entry = new VeritraceGlobalViewEntry(trace, name, startTime, endTime, quark); //We create an entry for this callpath 
		callpaths.put(id, entry); 
		List<Integer> children = ssq.getSubAttributes(quark, false); //And get the children

		for(Integer act : children) { //And we get the entry for every children as child of this entry
			TimeGraphEntry tmpEntry = recursiveCallpathRead(trace, endTime, endTime, ssq, act);	
			entry.addChild(tmpEntry);
		}
		return entry;
	}

	/**
	 * Create the right-click context menu for a particular entry
	 */
	@Override
	protected void fillTimeGraphEntryContextMenu( IMenuManager menuManager) {
		ISelection selection = getSite().getSelectionProvider().getSelection(); //We get which entry was selected by the user
		if (selection instanceof StructuredSelection) {
			StructuredSelection sSel = (StructuredSelection) selection;
			if (sSel.getFirstElement() instanceof VeritraceGlobalViewEntry) { //We check if the entry is one of our own
				VeritraceGlobalViewEntry entry = (VeritraceGlobalViewEntry) sSel.getFirstElement();
				if(entry.isVariable()) { //If the entry is a variable, it can have these menu
					//The option to add this variable to the ZoomView
					menuManager.add(new ZoomVariableAction(VeritraceGlobalView.this, entry.getName(), entry.getId(), entry.getTrace()));
					//The option to go open and highlight this variable location in it's source file
					String filename = entry.getFileName();
					if(!filename.isEmpty()) { //If we don't have the filename we don't display the button since we can't get to this variable in the source code
						menuManager.add(new OpenCodeFileAction(VeritraceGlobalView.this, entry.getName(), entry.getId(), entry.getFileName(), entry.getLine(), entry.getTrace()));
					}
				}

				//The option to remove the already saved path to enter a new one
				menuManager.add(new RemoveCodeFilePathAction(VeritraceGlobalView.this, entry.getTrace()));
			}
		}
	}

	/**
	 * Build the "lines" of the GlobalView and call the function to put information inside these "lines"
	 */
	@Override
	protected void buildEntryList( ITmfTrace trace,  ITmfTrace parentTrace,  IProgressMonitor monitor) {
		final ITmfStateSystem ssq = TmfStateSystemAnalysisModule.getStateSystem(trace, VeritraceAnalysisModule.ID);

		if(ssq == null) {//If their is no state system for this trace we can't obtain data to populate the GlobalView
			return;
		}

		Map<Integer, TimeGraphEntry> entryMap = new HashMap<>(); //The map used to store entries of variable
		TimeGraphEntry traceEntry = null; //The root entry

		long startTime = ssq.getStartTime();
		long start = startTime;
		setStartTime(Math.min(getStartTime(), startTime)); 		

		boolean complete = false;
		boolean doneOnceCallpath = false;
		while(!complete) {
			if(monitor.isCanceled()) {
				return;
			}
			complete = ssq.waitUntilBuilt(BUILD_UPDATE_TIMEOUT); //We wait on the StateSystem 
			if(ssq.isCancelled()) {
				return;
			}
			long end = ssq.getCurrentEndTime();
			if(start == end && !complete) {
				continue;
			}
			long endTime = end + 1;
			setEndTime(Math.max(getEndTime(), endTime));

			//We create or update the root entry
			if(traceEntry == null) {
				traceEntry = new VeritraceGlobalViewEntry(trace, startTime, endTime);
				List<TimeGraphEntry> entryList = Collections.singletonList(traceEntry);
				addToEntryList(parentTrace, entryList);
			}
			else {
				traceEntry.updateEndTime(endTime);
			}

			if(!doneOnceCallpath) { //While the StateSystem is being built, it happens that sometime the callpath would be shown multiple times, this only allow to do that once.
				//It should be fine since the callpath is stored at the beginning of a trace
				List<Integer> quarksCallpath = ssq.getQuarks(Callpath.EventRoot, "*");//we get every attribute stored under Callpath.EventRoot
				//Should only be done once for the main of the program because we should only have one root for the callpath
				for(Integer quark : quarksCallpath) {
					TimeGraphEntry entry = entryMap.get(quark); 
					if(entry == null) {
						TimeGraphEntry entryTmp = recursiveCallpathRead(trace, startTime, endTime, ssq, quark); //We call the function on the root of the callpath and it will recursively build the entries for all the callpath
						traceEntry.addChild(entryTmp); //and of course, we add the callpath root as child of the trace root entry
					}
				}
				doneOnceCallpath = true;
			}

			//We get the context for the variable from the State System
			List<Integer> quarkContext = ssq.getQuarks(Context.EventRoot, "*");//we get every attribute stored under Context.EventRoot
			for(Integer quarkCurrentContext : quarkContext) {
				Context context = Context.extractFromStateSystem(ssq, quarkCurrentContext);
				long id = Long.parseLong(ssq.getAttributeName(quarkCurrentContext));
				contexts.put(id, context); //and store it with a map with the keys being the context identifier (the hash of the variable from VeriTracer)
			}

			//We then create the entries for every value event of the trace
			List<Integer> quarkValues = ssq.getQuarks(Value.EventRoot, "*"); //we get every attribute stored under Value.EventRoot
			for(Integer quark : quarkValues) {
				TimeGraphEntry entry = entryMap.get(quark);
				if(entry == null) {//if the entry doesn't already exist
					//We identify the correct context for this variable
					long idContext = Long.parseLong(ssq.getAttributeName(quark));
					Context valueContext = contexts.get(idContext); 
					String variableName = valueContext.getName(); //And extract information from the context 
					int type = valueContext.getTypeSize();
					int line = valueContext.getLine();
					String file = valueContext.getFile();

					entry = new VeritraceGlobalViewEntry(trace, variableName, startTime, endTime, quark, type, line, file); //creating an entry for the variable

					entryMap.put(quark, entry);

					int parent_id = -1;
					try {
						int quarkParent = ssq.getQuarkRelative(quark, "parent");
						parent_id = ssq.querySingleState(ssq.getCurrentEndTime(), quarkParent).getStateValue().unboxInt(); //we get the callpath parent for this variable
					} catch (StateSystemDisposedException | AttributeNotFoundException e) {
						e.printStackTrace();
					}

					if(callpaths.containsKey(parent_id)) {
						callpaths.get(parent_id).addChild(entry); //We add the new entry as a child of the correct callpath's entry
					}
					else {
						traceEntry.addChild(entry); //If we do not have a corresponding callpath for the variable, it is added at the root
						//						System.err.println("The key doesn't exist for the variable " + variableName + " in the callpath.");
					}
				}
				else {
					entry.updateEndTime(endTime); //If the entry is already existing we update it's end time
				}
			}

			long resolution = Long.max(1, (end - start) / getDisplayWidth()); 
			for(Integer act: entryMap.keySet()) { //We go around entry representing variable to populate them
				ITimeGraphEntry child = entryMap.get(act);
				if (monitor.isCanceled()) {
					return;
				}
				if (child instanceof TimeGraphEntry) {
					populateTimeGraphEntry(monitor, start, endTime, resolution, (TimeGraphEntry) child);
				}
			}
			start = end;
		}

		if(parentTrace.equals(getTrace())) {
			refresh();
		}
	}

	@TmfSignalHandler
	@Override
	public void traceSelected(TmfTraceSelectedSignal signal) {
		IAction customFilterAction = getCustomFilterAction();
		if(customFilterAction instanceof CustomFilterAction && isCustomFilterEnabled) { //If the filter is enabled and we want to change the currently selected trace
			((CustomFilterAction) customFilterAction).removeFilter();
		}
		super.traceSelected(signal);
	}

	/**
	 * Add our own button in the top bar.
	 */
	@Override
	protected void fillLocalToolBar(IToolBarManager manager) {
		manager.appendToGroup(IWorkbenchActionConstants.MB_ADDITIONS, getCustomFilterAction()); //We add our button on the toolbar for the filter on significant_digits
		manager.appendToGroup(IWorkbenchActionConstants.MB_ADDITIONS, new Separator());
		super.fillLocalToolBar(manager); //And we add every other buttons
	}

	/**
	 * Add the time events on the entry
	 * @param monitor
	 * @param start the start time of the entry
	 * @param endTime the end time of the entry
	 * @param resolution the resolution 
	 * @param entry the entry needing it's time event added
	 */
	private void populateTimeGraphEntry( IProgressMonitor monitor, long start, long endTime, long resolution,  TimeGraphEntry entry) {
		List<ITimeEvent> eventList = getEventList(entry, start, endTime, resolution, monitor);
		if (eventList != null) {
			for (ITimeEvent event : eventList) {
				entry.addEvent(event);
			}
		}
		//		redraw();
	}


	/**
	 *  Gets the events for the entry from the StateSystem
	 */
	@Override
	protected  List< ITimeEvent> getEventList( TimeGraphEntry entry, long startTime, long endTime, long resolution,  IProgressMonitor monitor) {
		/* Length of the intervals queried */
		//		long bucketLength = 5 * resolution;
		long bucketLength = 2 * resolution;

		if (!(entry instanceof VeritraceGlobalViewEntry)) {
			/* Is a parent */
			return null;
		}

		VeritraceGlobalViewEntry VeritraceTimeGraphEntry = (VeritraceGlobalViewEntry) entry;
		ITmfStateSystem ss = TmfStateSystemAnalysisModule.getStateSystem(VeritraceTimeGraphEntry.getTrace(), VeritraceAnalysisModule.ID);
		if (ss == null) {
			return null;
		}

		int nbVariable;
		try {
			nbVariable = ss.getSubAttributes(ss.getQuarkAbsolute(Value.EventRoot), false).size();
			if (nbVariable == 0) {
				/* Can't get any information on the number of variable state */
				return null;
			}
		} catch (AttributeNotFoundException e) {
			e.printStackTrace();
		}

		/* Make sure the times are correct */
		final long realStart = Math.max(startTime, ss.getStartTime());
		final long realEnd = Math.min(endTime, ss.getCurrentEndTime());
		if (realEnd <= realStart) {
			return null;
		}

		/* Retrieve analysis module */
		VeritraceAnalysisModule veritraceAnalysisModule = TmfTraceUtils.getAnalysisModuleOfClass(VeritraceTimeGraphEntry.getTrace(), VeritraceAnalysisModule.class, VeritraceAnalysisModule.ID);
		if (veritraceAnalysisModule == null) {
			return null;
		}

		List<ITimeEvent> eventList = new ArrayList<>();
		long queryStart = realStart;
		long queryEnd = queryStart + bucketLength;


		/* Cover 100% of the width */
		while (queryStart <= realEnd) {
			if (monitor.isCanceled()) {
				return null;
			}

			Map<Integer, Double> map = veritraceAnalysisModule.getValuesRange(queryStart, queryEnd, "significant_digits"); 
			//We use the analysis module to extract the significant_digits from the state system

			if (map.containsKey(VeritraceTimeGraphEntry.getId())) { //if we have information about this entry
				Double significant_digits = map.get(VeritraceTimeGraphEntry.getId()); //we get the information about the entry
				if (significant_digits != null) {
					ITimeEvent event = new VeritraceTimeEvent(entry, queryStart, bucketLength, significant_digits); //and create an event for the data
					eventList.add(event);
					VeritraceTimeEvent castEvent = (VeritraceTimeEvent) event;
					if(fFilter != null && isCustomFilterEnabled) {
						if(fFilter.evaluate(castEvent.value)) {//decide if the event needs to be shown or not
							castEvent.setVisibility(true); //So we set the visibility of the event so that when the view is refreshed it will be display
						}
						else {
							castEvent.setVisibility(false); //or not displayed
						}
					}
				}
			}
			queryStart = queryEnd;
			queryEnd += bucketLength;
		}

		if (monitor.isCanceled()) {
			return null;
		}
		return eventList;
	}
}
