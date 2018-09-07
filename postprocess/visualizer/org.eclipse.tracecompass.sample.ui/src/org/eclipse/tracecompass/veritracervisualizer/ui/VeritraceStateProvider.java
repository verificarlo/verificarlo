package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.tracecompass.statesystem.core.ITmfStateSystemBuilder;
import org.eclipse.tracecompass.statesystem.core.statevalue.ITmfStateValue;
import org.eclipse.tracecompass.statesystem.core.statevalue.TmfStateValue;
import org.eclipse.tracecompass.tmf.core.event.ITmfEvent;
import org.eclipse.tracecompass.tmf.core.event.ITmfEventField;
import org.eclipse.tracecompass.tmf.core.statesystem.AbstractTmfStateProvider;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;

/**
 * Create the State System for a trace
 * @author Damien Thenot
 */
public class VeritraceStateProvider extends AbstractTmfStateProvider {

	private static final int VERSION = 42; //Change so that the State System is rebuilt and the state provider rerun with new changes
	public static final String ID = "org.eclipse.tracecompass.veritracervisualizer.VeritraceStateProvider"; //ID identifying it with the plugin
	private static Map<Integer, Callpath> callpaths = new HashMap<>(); //Map to remember the callpaths created
	private static Map<Long, Context> contexts = new HashMap<>(); //Map for the contexts

	/**
	 * Abstract class defining a handler for a type of event.
	 * To add a new handler, add one in createHandlers
	 * @author Damien Thenot
	 */
	private abstract class VeritraceEventHandler{
		public abstract void eventHandle(ITmfStateSystemBuilder ss, ITmfEvent data);
	}
	private Map<String, VeritraceEventHandler> handlers = null; //map of the defined events handlers 

	public VeritraceStateProvider(ITmfTrace trace) {
		super(trace, ID);
		createHandlers(); //Create the handlers for the events
	}

	public VeritraceStateProvider(ITmfTrace trace, int queueSize, int chunkSize) {
		super(trace, ID, queueSize, chunkSize);
		createHandlers();
	}

	@Override
	public VeritraceStateProvider getNewInstance() {
		return new VeritraceStateProvider(getTrace());
	}

	@Override
	public int getVersion() {
		return VERSION;
	}

	/**
	 * Create the different handlers for the different types of events in the trace.
	 */
	private void createHandlers(){
		handlers = new HashMap<>();
		handlers.put(Value.EventName, new VeritraceEventHandler() { //Handler for the value event
			@Override
			public void eventHandle(ITmfStateSystemBuilder ss, ITmfEvent data) {
				ITmfEventField content = data.getContent();
				long ts = data.getTimestamp().toNanos();
				long context = (long) content.getField("context").getValue();

				//We obtain the quark for the variables root
				int quarkVariables = ss.getQuarkAbsoluteAndAdd(Value.EventRoot);

				//We extract information from the content of the event
				Map<String, ITmfStateValue> contentVariable = Value.getContentVariableEvent(content);

				//We create the variable or find the existing entry
				int quarkVariable = ss.getQuarkRelativeAndAdd(quarkVariables, Long.toString(context));

				//We put the extracted information in the entry
				int quark;
				for(String act : contentVariable.keySet()) {
					quark = ss.getQuarkRelativeAndAdd(quarkVariable, act);
					ss.modifyAttribute(ts, contentVariable.get(act), quark);
				}
			}
		});

		handlers.put(Callpath.EventName, new VeritraceEventHandler() {
			@Override
			public void eventHandle(ITmfStateSystemBuilder ss, ITmfEvent data) {
				ITmfEventField content = data.getContent();
				//We put the information on the callpath from the beginning of the trace since these informations don't really have a timestamp
				long ts = ss.getStartTime(); 

				int id_parent = Math.toIntExact((long) content.getField("parent").getValue());
				int id = Math.toIntExact((long) content.getField("id").getValue());
				String name = (String) content.getField("name").getValue();

				ITmfStateValue value = TmfStateValue.newValueInt(id);

				//Create the backtrace tree
				int quarkContext = ss.getQuarkAbsoluteAndAdd(Callpath.EventRoot);
				int quark = -1;
				if(id_parent == 0) {
					quark = ss.getQuarkRelativeAndAdd(quarkContext, name);
				}
				else {
					//We use the callpath map indexed by the unique identifier of the callpath context to retrieve the quark of the parent
					int quarkParent = callpaths.get(id_parent).getQuark();
					quark = ss.getQuarkRelativeAndAdd(quarkParent, name);
				}

				try {
					//The callpath context identifier is stored inside the state
					ss.modifyAttribute(ts, value, quark);
				}
				catch (IndexOutOfBoundsException e) {}
				
				//The new callpath context is added to the known callpath context with it's identifier as the retrieval key
				callpaths.put(id, new Callpath(id_parent, name, id, quark));
			}
		});

		handlers.put(Context.EventName, new VeritraceEventHandler() {
			@Override
			public void eventHandle(ITmfStateSystemBuilder ss, ITmfEvent data) {
				ITmfEventField content = data.getContent();
				//We put the context informations at the beginning since they are not time dependent event
				long ts = ss.getStartTime();

				long id = (long) content.getField("id").getValue();
				String file = (String) content.getField("file").getValue();
				String function = (String) content.getField("function").getValue();
				int line = Math.toIntExact((long) content.getField("line").getValue());
				int type = Integer.parseInt((String) content.getField("type").getValue());
				String name = (String) content.getField("name").getValue();
				
				int quarkContextRoot = ss.getQuarkAbsoluteAndAdd(Context.EventRoot);
				int quarkContext = -1;

				Map<String, ITmfStateValue> contentVariable = Context.getContentContextEvent(content);

				quarkContext = ss.getQuarkRelativeAndAdd(quarkContextRoot, Long.toString(id));

				int quark = -1;
				for(String act : contentVariable.keySet()) {
					quark = ss.getQuarkRelativeAndAdd(quarkContext, act);
					ss.modifyAttribute(ts, contentVariable.get(act), quark);
				}
				//The new callpath context is added to the known callpath context with it's identifier as the retrieval key
				contexts.put(id, new Context(id, file, function, line, name, type));
			}
		});
	}

	/**
	 * Called for each event of a trace, decide which handler to use for an event
	 */
	@Override
	protected void eventHandle(ITmfEvent data) {
		ITmfStateSystemBuilder ss = getStateSystemBuilder();
		//Differentiate the type of events
		String handler = data.getName();
		if(handlers.containsKey(handler)) {
			handlers.get(handler).eventHandle(ss, data);
		}
		else {
			System.err.println("Event of type " + handler + " is not a valid event type.");
		}
	}
}
