package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.tracecompass.statesystem.core.ITmfStateSystem;
import org.eclipse.tracecompass.statesystem.core.exceptions.AttributeNotFoundException;
import org.eclipse.tracecompass.statesystem.core.exceptions.StateSystemDisposedException;
import org.eclipse.tracecompass.statesystem.core.statevalue.ITmfStateValue;
import org.eclipse.tracecompass.statesystem.core.statevalue.TmfStateValue;
import org.eclipse.tracecompass.tmf.core.event.ITmfEventField;

/**
 * Class allowing to represent a context and obtain it from the StateSystem.
 * A context is containing information on a variable it is linked to with an unique identifier, usually the variable hash created by VeriTracer
 * @author Damien Thenot
 *
 */
public class Context {
	//The name of the root attribute in the state system
	public static final String EventRoot = "contexts";
	//The name of the type of event in the CTF traces
	public static final String EventName = "context";
	
	//The name of single precision floating-point variable
	private static final String SINGLE_NAME = "binary32";
	//The name of double precision floating-point variable
	private static final String DOUBLE_NAME = "binary64";

	private long id; //The identifier of the context, the hash of the variable usually
	private String file; //The name (or path and name) of the source file containing the variable of this context
	private String function; //Function of the variable of this context
	private int line; //Line of the variable in the source file
	private String name; //Name of the variable
	private int type; //Size in bytes of the variable

	//Name of the fields containing integer in the CTF trace Context event's content
	private static final String[] contentNameInt = new String[] {
			"line"
	};

	//Fields containing String in the context event in the CTF trace
	private static final String[] contentNameString = new String[] {
			"file",
			"function",
			"name",
			"type"
	};

	public Context(long id, String file, String function, int line, String name, int type) {
		this.id = id;
		this.file = file;
		this.function = function;
		this.line = line;
		this.name = name;
		this.type = type;
	}

	public long getId() {
		return id;
	}

	public String getFile() {
		return file;
	}

	public String getFunction() {
		return function;
	}

	/**
	 * Return the line in the original code
	 * @return the line number
	 */
	public int getLine() {
		return line;
	}

	public String getName() {
		return name;
	}

	/**
	 * Return the size in byte of the variable of this context
	 */
	public int getTypeSize() {
		return type;
	}
	
	/**
	 * Return the name of the type for this size
	 * @param type the size in byte to transform in showable name
	 * @return printable name for the type 
	 */
	public static String getTypeName(int type) {
		switch(type) {
		case 4:
			return SINGLE_NAME;
		case 8:
			return DOUBLE_NAME;
		default:
			return "";
		}
	}

	/**
	 * Return a map containing the ITmfStateValue object to insert in the state system, 
	 * it extracts the data and create a TmfStateValue of the corresponding type
	 * @param content content of a Context Event from a CTF trace
	 */
	public static Map<String, ITmfStateValue> getContentContextEvent(ITmfEventField content) {
		Map<String, ITmfStateValue> eventValues = new HashMap<>();

		int eventValueInt;
		for(String act : contentNameInt) { //extract every integer type item from the content of the event
			//contentNameInt is an array containing the name of every field containing integer type value 
			eventValueInt = Math.toIntExact((long) content.getField(act).getValue());
			eventValues.put(act, TmfStateValue.newValueInt(eventValueInt));
		}

		String eventValueString; 
		for(String act : contentNameString) { //extract string type
			eventValueString = (String) content.getField(act).getValue();
			eventValues.put(act, TmfStateValue.newValueString(eventValueString));
		}

		return eventValues;
	}

	/**
	 * Extract a context from the StateSystem
	 * @param ss the state system
	 * @param quark the root quark of the context in the state system 
	 * @return a new Context object containing the informations extracted from the state system
	 */
	public static Context extractFromStateSystem(ITmfStateSystem ss, Integer quark) {
		long ts = ss.getCurrentEndTime(); //We extract from the very end of the state system, the context in the state system should never change in time so that should not create problems
		long id = Long.parseLong(ss.getAttributeName(quark)); //the hash of the variable of this context
		try {
			int quarkChild = ss.getQuarkRelative(quark, "file");
			String file = ss.querySingleState(ts, quarkChild).getStateValue().unboxStr();
			
			quarkChild = ss.getQuarkRelative(quark, "function");
			String function = ss.querySingleState(ts, quarkChild).getStateValue().unboxStr();
			
			quarkChild = ss.getQuarkRelative(quark, "line");
			int line = ss.querySingleState(ts, quarkChild).getStateValue().unboxInt();
			
			quarkChild = ss.getQuarkRelative(quark, "name");
			String name = ss.querySingleState(ts, quarkChild).getStateValue().unboxStr();
			
			quarkChild = ss.getQuarkRelative(quark, "type");
			int type = Integer.parseInt(ss.querySingleState(ts, quarkChild).getStateValue().unboxStr());
			
			return new Context(id, file, function, line, name, type);
		} catch (StateSystemDisposedException | AttributeNotFoundException e) {
			e.printStackTrace();
		}
		return null;
	}
}
