package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.tracecompass.tmf.ctf.core.event.CtfTmfEvent;

/**
 * Meant to define ITmfEvent for Veritracer, it is not used but need to be defined for the tracetype in the plugin.
 */
@SuppressWarnings("deprecation")
public class VeritraceEvent extends CtfTmfEvent {
	public long getContext() {
		return (long) getContent().getField("context").getValue();
	}
}
