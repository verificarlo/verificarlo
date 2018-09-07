package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.Iterator;

import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeEvent;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.TimeGraphEntry;

/**
 * An entry inside the TimeGraph on the right of the GlobalView
 * contains {@link VeritraceTimeEvent} representing event in time
 * @author Damien Thenot
 * @see org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.TimeGraphEntry
 */
public class VeritraceGlobalViewEntry extends TimeGraphEntry {
	private static final int NO_ID = -666;
	
	private final int fId;
	private final ITmfTrace fTrace;

	private int fType;
	private int fLine;
	private String fFile;
	
	private boolean isVariable;

	/**
	 * Constructor used for non-variable in the treeview
	 * @param trace ITmfTrace
	 * @param startTime long
	 * @param endTime long
	 */
	public VeritraceGlobalViewEntry(ITmfTrace trace, long startTime, long endTime) {
		this(trace, trace.getName(), startTime, endTime, NO_ID, 0, -1, "");
		isVariable = false;
	}
	
	/**
	 * Constructor used for variables in the treeview
	 * @param trace ITmfTrace
	 * @param name String
	 * @param startTime long
	 * @param endTime long
	 * @param id int
	 */
	public VeritraceGlobalViewEntry(ITmfTrace trace, String name, long startTime, long endTime, int id) {
		super(name, startTime, endTime);
		fTrace = trace;
		fId = id;
		fType = -1;
		isVariable = false;
		fLine = -1;
	}
	
	/**
	 * Constructor with the type of the variable represented by this entry
	 * @param trace ITmfTrace The trace containing this entry
	 * @param name String The name of the entry (name of the variable in this case)
	 * @param startTime long
	 * @param endTime long
	 * @param id int
	 * @param type int Type of the variable, a number of byte
	 * @param line int
	 */
	public VeritraceGlobalViewEntry(ITmfTrace trace, String name, long startTime, long endTime, int id, int type, int line, String file) {
		super(name, startTime, endTime);
		fTrace = trace;
		fId = id;
		fType = type;
		isVariable = true;
		fLine = line;
		fFile = file;
	}

	public int getId() {
		return fId;
	}
	
	public ITmfTrace getTrace() {
		return fTrace;
	}
	
	/**
	 * 
	 * @return true if the entry has an ID, false otherwise
	 */
	public boolean hasId() {
		return fId != NO_ID;
	}

	/**
	 * Return the size of the variable in byte
	 * @return int
	 */
	public int getType() {
		return fType;
	}
	
	
	/**
	 * Return the name of the type of the variable 
	 * @return String
	 */
	public String getTypeName() {
		return Context.getTypeName(fType);
	}

	/**
	 * Return true if the entry is representing a variable, false otherwise
	 * @return boolean
	 */
	public boolean isVariable() {
		return isVariable;
	}

	/**
	 * Return the line in the original file
	 * @return int
	 */
	public int getLine() {
		return fLine;
	}

	public String getFileName() {
		return fFile;
	}

	/**
	 * If the entry contains events where the number of significants digits < f return true else return false
	 * @param f double the limit under which an event needs not to be to be considered not problematic
	 * @return boolean true if any of the {@link VeritraceTimeEvent} are below f, other {@link ITimeEvent} are ignored
	 */
	public boolean isProblematic(double f) {
		Iterator<ITimeEvent> iterator = getTimeEventsIterator(); //iterator on the ITimeEvent
		if(iterator != null) { 
			while(iterator.hasNext()) { //We browse every TimeEvent
				ITimeEvent act = iterator.next();
				if(act instanceof VeritraceTimeEvent) {
					VeritraceTimeEvent castAct = (VeritraceTimeEvent) act;
					if(castAct.value < f) { //if the VeritraceTimeEvent is < f, we return true
						return true;
					}
				}
			}
		}
		return false;
	}
}
