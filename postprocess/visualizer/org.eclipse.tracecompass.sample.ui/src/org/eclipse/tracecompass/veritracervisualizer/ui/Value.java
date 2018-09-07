package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.tracecompass.statesystem.core.statevalue.ITmfStateValue;
import org.eclipse.tracecompass.statesystem.core.statevalue.TmfStateValue;
import org.eclipse.tracecompass.tmf.core.event.ITmfEventField;


/**
 * Class representing a Value event from the CTF trace.
 * Contain everything needed to read it directly from the trace and put it in the State System.
 * @author Damien Thenot
 *
 */
public class Value {
	private String name; // The name of the variable this value event is part of
	private long context; //The identifier of the context containing information about this variable
	private int quark; //The identifier of this value in the State System.
	private int parent; //The identifier of this value callpath
	

	private static final String[] contentNameDouble = new String[] { //The name of the fields containing double value in the content of a Value event
			"mean",
			"min",
			"max",
			"median",
			"std",
			"significant_digits"
	};
	
	private static final String[] contentNameInt = new String[] { //Fields containing integer
			"parent"
	};
	
	private static final String[] contentNameLong = new String[] { //Fields containing long
			"context"
	};
	
	//The name of the root attributes in the file system for variable/value
	public static final String EventRoot = "values";
	//The name of the Value event in the CTF trace
	public static final String EventName = "value";
	
	/**
	 * Extract the values from the defined fields from the content of an event, it is used to insert the content in the state system
	 * @param content ITmfEventField
	 * @return a map containing the TmfStateValue of the variable event
	 */
	public static Map<String, ITmfStateValue> getContentVariableEvent(ITmfEventField content){
		Map<String, ITmfStateValue> eventValues = new HashMap<>();
		
		
		double eventValue; //For every field of type double in the content of the value event
		for(String act : contentNameDouble) {
			eventValue = (double) content.getField(act).getValue(); //We extract the value from the content
			eventValues.put(act, TmfStateValue.newValueDouble(eventValue)); //And create a TmfStateValue for this one in the map where the key is the name of the field
		}
		
		int eventValueInt;
		for(String act : contentNameInt) {
			eventValueInt = Math.toIntExact((long) content.getField(act).getValue());
			eventValues.put(act, TmfStateValue.newValueInt(eventValueInt));
		}
		
		long eventValueLong;
		for(String act : contentNameLong) {
			eventValueLong = (long) content.getField(act).getValue();
			eventValues.put(act, TmfStateValue.newValueLong(eventValueLong));
		}
		
		return eventValues; //The Map is then used to add it in the State System in VeritraceStateProvider
	}
	
	
	public Value(long context, int parent, String name, int quark) {
		this.context = context;
		this.parent = parent;
		this.name = name;
		this.quark = quark;
	}

	public String getName() {
		return name;
	}
	
	public void setName(String name) {
		this.name = name;
	}
	
	public int getParent() {
		return parent;
	}
	
	public long getContext() {
		return context;
	}
	
	public String toString() {
		return "Variable : Context : " + context + ", Name : " + name;
	}

	public int getQuark() {
		return quark;
	}
}
