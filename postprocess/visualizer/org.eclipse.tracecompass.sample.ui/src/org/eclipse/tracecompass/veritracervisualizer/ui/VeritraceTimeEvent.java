package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeGraphEntry;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.TimeEvent;

/**
 * Define a TimeEvent which is part of an entry in a TimeGraph
 * @author Damien Thenot
 */
public class VeritraceTimeEvent extends TimeEvent {
	public final double value; //The value of the event, the number of significant digits most likely
	private boolean isVisible; //If the TimeEvent should be visible on the TimeGraph

	public VeritraceTimeEvent(ITimeGraphEntry entry, long time, long duration) {
		super(entry, time, duration);
		value = Double.MIN_VALUE;
		isVisible = true;
	}
	
	public VeritraceTimeEvent(ITimeGraphEntry entry, long time, long duration, double value) {
		super(entry, time, duration);
		this.value = value;
		isVisible = true;
	}

	public VeritraceTimeEvent(ITimeGraphEntry entry, long time, long duration, int value) {
		super(entry, time, duration, value);
		this.value = Double.MIN_VALUE;
		isVisible = true;
	}
	
	/**
	 * @return true if the TimeEvent should be visible on the TimeGraph, false otherwise
	 */
	public boolean isVisible() {
		return isVisible;
	}

	/**
	 * @param b set to true if the TimeEvent should be visible on the TimeGraph, or false if not
	 */
	public void setVisibility(boolean b) {
		isVisible = b;		
	}
}
