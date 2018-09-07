package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.LinkedHashMap;
import java.util.Map;

import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.StateItem;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.TimeGraphPresentationProvider;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeEvent;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.NullTimeEvent;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.TimeEvent;

/**
 * Presentation provider give the style of the timegraph and give the color of a specific {@link VeritraceTimeEvent}.
 * @author Damien Thenot
 */
public class VeritraceGlobalViewPresentationProvider extends TimeGraphPresentationProvider {
	private ColorMap fColorMap; //The color map used to obtain a color
	private int stateSize; //The number of different state a TimeEvent can be
				
	public VeritraceGlobalViewPresentationProvider() {
		super();
		fColorMap = new ColorMap(0, 100); //We create a ColorMap with 100 different colors
		stateSize = 101; // There is a state for every color and one for not printed
	}
	
	/**
	 * Give the index in the state table for an event
	 * @param event the event we need to obtain the state of
	 * @return an index in the state table ∈ [-1 ; 100]
	 */
	protected int getEventIndex(TimeEvent event) {
		if(event instanceof VeritraceTimeEvent) {
			VeritraceTimeEvent tevent = (VeritraceTimeEvent) event;
			VeritraceGlobalViewEntry entry = (VeritraceGlobalViewEntry) event.getEntry();
			if(entry.isVariable()) { //if the entry is a variable the events needs to be colored
				if(tevent.isVisible()) { //If the event should be shown to the user
					if(tevent.value < -10) { 
						return -1;
					}
					else if(tevent.value < 0) {
						return 0; // we choose to define 0 as red and put every event with a significant_digits < 0 as 0
					}
					else {
						int i = -1;
						if(entry.getType() == 4) { //We normalize the value for the event by the type of the variable
							double valueNormalized = ColorMap.normalize(0, 8, tevent.value);
							i = ((Double) (valueNormalized * 100)).intValue();
							//We are using a precision of 10^2 to decide the color this event should be printed with
						}
						else if(entry.getType() == 8) {
							double valueNormalized = ColorMap.normalize(0, 17, tevent.value);
							i = ((Double) (valueNormalized * 100)).intValue();
						}
						return i;
					}	
				}
				else { //If the event should not be displayed to the user/invisible
					return -1;
				}
			}
		}
		return -1;
	}
	
	@Override
	public int getStateTableIndex(ITimeEvent event) {
		VeritraceGlobalViewEntry entry = (VeritraceGlobalViewEntry) event.getEntry();
		
		if (!entry.isVariable()) {
            return TRANSPARENT;
        }
		
		int state = getEventIndex((TimeEvent) event);
        if (state == -1) {
            return INVISIBLE;
        }
        
        if (state != -1) {
            return state;
        }
        
        if (event instanceof NullTimeEvent) {
            return INVISIBLE;
        }
        
        return TRANSPARENT;
	}
	
	/**
	 * Create the state table
	 */
	@Override
    public StateItem[] getStateTable() {
		
        StateItem[] stateTable = new StateItem[stateSize];
        
        for (int i = 0; i < stateTable.length; i++) {
        	stateTable[i] = new StateItem(fColorMap.getColor(i), Integer.toString(i) + "%");
        	//StateItem(RGB, String) : String being the name of the State in the legend of the view
        }
        return stateTable;
    }

	@Override
    public Map<String, Object> getSpecificEventStyle(ITimeEvent event) {
        Map<String, Object> specificEventStyle = super.getSpecificEventStyle(event);
        //TODO: Look for custom style
//        if (event instanceof VeritraceTimeEvent) {
//            VeritraceTimeEvent csEvent = (VeritraceTimeEvent) event;
//            if (csEvent.getEntry() instanceof VeritraceGlobalViewEntry) {
//                VeritraceGlobalViewEntry csEntry = (VeritraceGlobalViewEntry) csEvent.getEntry();
//                double mean = csEntry.getMean();
//                if (mean != 0.0) {
//                    Map<String, Object> retVal = new HashMap<>();
//                    int count = csEvent.getCount();
//                    float heightFactor = (float) (csEvent.getValue() / mean / count * 0.33);
//                    heightFactor = (float) Math.max(0.1f, Math.min(heightFactor, 1.0));
//                    retVal.put(ITimeEventStyleStrings.heightFactor(), heightFactor);
//                    return retVal;
//                }
//            }
//        }
        return specificEventStyle;
    }
	
	
	/**
	 * Create the event hover info
	 */
	@Override
    public Map<String, String> getEventHoverToolTipInfo(ITimeEvent event, long hoverTime) {
        Map<String, String> retMap = super.getEventHoverToolTipInfo(event, hoverTime);
        if(retMap == null) {
        	retMap = new LinkedHashMap<>();
        }
        if (event instanceof VeritraceTimeEvent) {
            VeritraceTimeEvent tEvent = (VeritraceTimeEvent) event;
            VeritraceGlobalViewEntry entry = (VeritraceGlobalViewEntry) event.getEntry();
            retMap.put("Name", entry.getName());
            if (tEvent.hasValue()) {
                retMap.put("Value", Double.toString(tEvent.getValue()));
            }
        }
        return retMap;
    }
}
