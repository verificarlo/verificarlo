package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.text.DecimalFormat;
import java.text.FieldPosition;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.tracecompass.statesystem.core.ITmfStateSystem;
import org.eclipse.tracecompass.statesystem.core.exceptions.AttributeNotFoundException;
import org.eclipse.tracecompass.tmf.core.signal.TmfSignalHandler;
import org.eclipse.tracecompass.tmf.core.signal.TmfTimestampFormatUpdateSignal;
import org.eclipse.tracecompass.tmf.core.statesystem.TmfStateSystemAnalysisModule;
import org.eclipse.tracecompass.tmf.core.timestamp.TmfTimestampFormat;
import org.eclipse.tracecompass.tmf.core.trace.ITmfTrace;
import org.eclipse.tracecompass.tmf.core.trace.TmfTraceUtils;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxis.Position;
import org.swtchart.ILineSeries;
import org.swtchart.ILineSeries.PlotSymbolType;
import org.swtchart.ISeries.SeriesType;
import org.swtchart.Range;

public class VeritraceZoomViewChart {
	
	private static final Map<String, String> fieldsClean; //a map containing nice name to be displayed in the Y Axis
	static {
		Map <String, String> map = new HashMap<>();
		map.put("significant_digits", "Significant Digits");
		map.put("max", "Max");
		map.put("min", "Min");
		map.put("std", "Standard Deviation");
		map.put("median", "Median");
		map.put("mean", "Mean");

		fieldsClean = Collections.unmodifiableMap(map);
	}
	
	private static final Map<String, Color> colorSeries; //the colors for a series' field
	static {
		Map <String, Color> map = new HashMap<>();
		map.put("significant_digits", new Color(Display.getDefault(), 0, 0, 255));
		map.put("max", new Color(Display.getDefault(), 255, 0, 255));
		map.put("min", new Color(Display.getDefault(), 255, 128, 0));
		map.put("std", new Color(Display.getDefault(), 0, 191, 255));
		map.put("median", new Color(Display.getDefault(), 255, 69, 0));
		map.put("mean", new Color(Display.getDefault(), 0, 255, 0));

		colorSeries = Collections.unmodifiableMap(map);
	}
	
	

	
	private static final long BUILD_UPDATE_TIMEOUT = 500L; //The time between two verification of the build state of the State System
	
	private static final String originalField = "significant_digits"; // The name of the field that we want to originally display on the Y axis
	private static final String X_AXIS_TITLE = "Time"; //The X axis name
	
	private long startTime; //the start time of this chart
	private long endTime; //The end time of this chart
	private long time0; //The t0 of the x axis to normalize the time
	private VeritraceZoomViewZoomProvider zoomProvider; //The listeners handling the Zoom functionalities
	private Set<String> fieldToDisplay; //The fields of the variable getting displayed in the chart
	private boolean isLeft; //Allows to alternate between left and right when adding a new axis
	private int variableQuark; //The quark of the variable in the StateSystem that this chart is showing informations on
	private String variableName;//The name of the variable this chart show
	private Map<String, Integer> axisId = new HashMap<>(); //A map allowing to know the id of an axis in the chart for a field
	private VeritraceZoomView zoomView; //The VeritraceZoomView parent instance
	private boolean logScale = false; //If the logscale is enabled on this chart
	private Chart chart; //The SWT Chart
	private boolean rangeToggled = true; //If the chart should accept a zoom signal
	private MouseMoveListener mouseMoveListener = null; //The listener for the hover informations
	private ITmfTrace fTrace; //The trace the variable is from
	
	public VeritraceZoomViewChart(Composite fParent, VeritraceZoomView zoomView, ITmfTrace trace, String variableName, int variableQuark, Set<String> fieldToDisplay) {
		Chart chart = createChart(fParent, variableName, variableQuark);
		this.chart = chart;
		
		axisId.put(originalField, 0); //The YAxis 0 cannot be deleted, there is always at least one axis, so we define one field as having it as Axis.
		
		this.fieldToDisplay = fieldToDisplay;
		this.fTrace = trace;
		isLeft = true;
		this.variableName = variableName;
		this.variableQuark = variableQuark;
		this.zoomView = zoomView;
		
		setStartTime(trace.getStartTime().toNanos());
		setEndTime(trace.getEndTime().toNanos());
		
		zoomProvider = new VeritraceZoomViewZoomProvider(zoomView, chart, fieldToDisplay); //We had a zoom provider to this newly created chart
	}
	
	/**
	 * Create the SWT Chart object and configure it to our needs
	 * @param parent the Composite parent hosting the chart
	 * @param variableName the name of the variable this chart is representing
	 * @param variableQuark the quark in the StateSystem of the variable
	 * @return the newly created chart
	 */
	private Chart createChart(Composite parent, String variableName, int variableQuark) {
		Chart chart = new Chart(parent, SWT.BORDER);

		Menu menu = createMenu(parent, variableName, variableQuark); //Context menu for the chart
		chart.setMenu(menu); 
		chart.getPlotArea().setMenu(menu);
		
		chart.getTitle().setText(variableName); //Naming of the chart with the variable name
		IAxis xAxis = chart.getAxisSet().getXAxis(0);
		xAxis.getTitle().setText(X_AXIS_TITLE); //The X Axis name
		IAxis yAxis = chart.getAxisSet().getYAxis(0);
		yAxis.getTitle().setText(""); //The Y axis is hidden until we have data
		yAxis.getTitle().setVisible(false); 
		yAxis.getTick().setVisible(false); //We hide the axis of the original field in case it isn't needed to be shown on this chart
		chart.getLegend().setVisible(false); //The legend is hidden by default since there is only one series for each axis
		//			chart.getAxisSet().getXAxis(0).getTick().setFormat(new TmfChartTimeStampFormat());
		xAxis.getTick().setFormat(new DecimalFormat("0.0E0")); //Set the X axis tick format to a scientific format
//		chart.getAxisSet().getYAxis(0).getTick().setFormat(new DecimalFormat("0.0000"));
		return chart;
	}
	
	/**
	 * Create the right-click context menu on the charts.
	 * @param parent the parent window
	 * @param nameVariable the name of the variable the menu is for 
	 * @param quarkVariable the quark of the variable the menu is for
	 * @return the newly created menu
	 */
	private Menu createMenu(Composite parent, final String nameVariable, final int quarkVariable) {
		Menu menu = new Menu(parent.getShell(), SWT.POP_UP);

		MenuItem item = new MenuItem(menu, SWT.PUSH); //Button to remove a variable from the ZoomView directly from the ZoomView by right-clicking the chart
		item.setText("Remove from Zoom View"); 
		item.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				zoomView.broadcast(new VariableSelectedSignal(this, fTrace,nameVariable, quarkVariable));
				//VariableSelectedSignal is a toggle on the ZoomView so sending a signal on the variable with it's name and quark works
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});
		
		item = new MenuItem(menu, SWT.CHECK);
		item.setText("Toggle Zoom");
		item.setSelection(rangeToggled);
		item.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				rangeToggled = !rangeToggled;
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e) {
				
			}
			
		});
		
		item = new MenuItem(menu, SWT.CHECK);
		item.setText("Enable logScale");
		item.setSelection(isLogScale());
		item.addSelectionListener(new SelectionListener() {

			@Override
			public void widgetSelected(SelectionEvent e) {
				logScale = !logScale; //We change the status of the logscale
				/*For every axis, we have to update the axis' logscale state to ensure the axis is correctly 
				 * represented before loading data, 
				 * otherwise the logscale would be disabled after having updated the data and the range
				 * and would be wrong.
				 */
				IAxis[] yAxis = chart.getAxisSet().getYAxes();
				for(int i = 0; i < yAxis.length; i++) {
					yAxis[i].enableLogScale(false);
				}
				populateChart(); //We update the chart with the new setting
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e) {}
			
		});
		return menu;	
	}
	
	public void setStartTime(long time) {
		this.startTime = time;
	}
	
	public void setEndTime(long time) {
		this.endTime = time;
	}
	
	public long getStartTime() {
		return this.startTime;
	}
	
	
	public long getEndTime() {
		return this.endTime;
	}
	
	public void setTime(long startTime, long endTime) {
		setStartTime(startTime);
		setEndTime(endTime);
	}
	
	/**
	 * Dispose of the objects composing this object
	 */
	public void dispose() {
		zoomProvider.deregister();
		chart.dispose();
	}
	
	/**
	 * Give the series for a specific chart with a specific name, create and initialize it if needed
	 * @param chart the chart we want the series from
	 * @param seriesName the name of the series we want, it is the name of one the fields
	 * @return the series
	 */
	private ILineSeries getSeries(String seriesName) {
		ILineSeries series = (ILineSeries) chart.getSeriesSet().getSeries(seriesName); //We get the existing series
		
		if(series == null) { //If there is no existing series for this field, we create it
			series = (ILineSeries) chart.getSeriesSet().createSeries(SeriesType.LINE, seriesName);
			series.setSymbolType(PlotSymbolType.NONE);
			series.setLineColor(colorSeries.get(seriesName));
		}
		series.setDescription(getPrintableName(seriesName));
		return series;
	}
	
	/**
	 * Return the printable name defined in the fieldsClean map for name
	 * @param name the name we want displayable
	 * @return the displayable name
	 */
	private String getPrintableName(String name) {
		String printableName = VeritraceZoomViewChart.getfieldsClean().get(name);
		if(isLogScale()) { //If the name need to be shown as representing the absolute of all values
			return getAbsoluteName(printableName);
		}
		else {
			return printableName;
		}
	}
	
	/**
	 * Give name with absolute symbol, |name|
	 * @return the string name with absolute symbol |name|
	 */
	private String getAbsoluteName(String name) {
		return "|" + name + "|";
	}
	
	/**
	 * Return, and create if needed, the Y axis for the field act
	 * @param chart the chart this axis is part of
	 * @param field the field this axis is for
	 * @return the identifier of the axis in the chart.getAxisSet().getYAxis(id)
	 */
	private int getAxis(String field) {		
			if(!axisId.containsKey(field)) { //If the axis for this field doesn't already exist
				int newAxis = chart.getAxisSet().createYAxis(); //We create the axis on the chart and get it's id
				IAxis axis = chart.getAxisSet().getYAxis(newAxis); //We get the object of this new axis
				Color colorAxis = colorSeries.get(field); //We get the color for this field
				axis.getTick().setForeground(colorAxis);
				axis.getTitle().setForeground(colorAxis); //And apply it to the axis
				
				if(isLeft) { //if the last axis to be added was on the left
					axis.setPosition(Position.Secondary); //We set the position of the axis on the right
					isLeft = false;
				}
				else { //or if the last axis to be added was on the right
					axis.setPosition(Position.Primary); //We set the position of the axis on the left
					isLeft = true;
				}
				axisId.put(field, newAxis); //We add the id of this axis on the YAxis of the chart to the known axis
			}
			return axisId.get(field); //We return the id of the wanted axis on the chart
	}
	

	/**
	 * Remove the axis for act from chart
	 * @param chart the chart the axis will be deleted from
	 * @param field the field of the axis needed to be destroy
	 */
	public void removeAxis(String field) {
			if(axisId.containsKey(field)) { //If the field has an axis
				int id = axisId.get(field);

				if(id == 0) { //The axis 0 is not disposable, it need to stay
					IAxis axis = chart.getAxisSet().getYAxis(id);	
					axis.getTitle().setText(""); //We still hide the axis if it doesn't need to be shown
					axis.getTick().setVisible(false);
				}
				else{
					chart.getAxisSet().deleteYAxis(id);
					axisId.remove(field);
					isLeft = !isLeft;
				}
			}
	}
	

	/**
	 * Destroy the series of the field seriesName from chart
	 * @param chart the chart we act upon
	 * @param seriesName the name of the field and series we wish to destroy
	 */
	public void removeSeries(String seriesName) {
		chart.getSeriesSet().deleteSeries(seriesName);
	}

	/**
	 * Action to perform when we receive a {@link TmfTimestampFormatUpdateSignal}
	 * @param signal the signal
	 */
	@TmfSignalHandler
	public void timestampFormatUpdated(TmfTimestampFormatUpdateSignal signal) {
		// Called when the time stamp preference is changed
			chart.getAxisSet().getXAxis(0).getTick().setFormat(new TmfChartTimeStampFormat());
			chart.redraw();
	}
	
	/**
	 * A TimeFormat initially used for the X Axis.
	 */
	public class TmfChartTimeStampFormat extends SimpleDateFormat {
		private static final long serialVersionUID = 1L;
		@Override
		public StringBuffer format(Date date, StringBuffer toAppendTo, FieldPosition fieldPosition) {
			long time = date.getTime();
			toAppendTo.append(TmfTimestampFormat.getDefaulTimeFormat().format(time));
			return toAppendTo;
		}
	}

	/**
	 * A static getter for the fieldsClean map so that the other needing to print clean field names can access it
	 */
	public static Map<String, String> getfieldsClean() {
		return fieldsClean;
	}
	

	/**
	 * @return the state of the logarithmic scale on this chart
	 */
	public boolean isLogScale() {
		return logScale;
	}
	
	/**
	 * Populate the series for every fields needing to be displayed 
	 * @param chart
	 * @param variableQuark
	 * @return 0 if success, 1 if error
	 */
	protected int populateChart() {
		for(String field : fieldToDisplay){ //every field the user want displayed
			ILineSeries series = getSeries(field); //The series we are working on
			int axisId = getAxis(field); //the axis of the field
			
			if(axisId == -1) {
				return 1;
			}
			
			ChartToPrint res = getValuesField(variableQuark, field); //We obtain the data for the chart for the field
			
			if(res == null ) {
				return 1;
			}
			
			ArrayList<Double> xValues = res.getxValues(), yValues = res.getyValues();
			final double minX = res.getMinX(), minY = res.getMinY(), maxX = res.getMaxX(), maxY = res.getMaxY();
			time0 = (long) minX;
			zoomProvider.setTime0((long) minX); //We update the time0 of the ZoomProvider so that it can correctly compute the time for a zoom
			
			if(logScale) { //if the logarithmic scale has been enabled, we make the data of the Y axis positive
				yValues = getAbsoluteArray(yValues);
			}

			//		final double x[] = toArray(xValues);
			final double x[] = toArrayNormalized(xValues, minX); //Allow time normalized to 0
			final double y[] = toArray(yValues);
			
			chart.getAxisSet().getXAxis(0).getTitle().setText(X_AXIS_TITLE + " in nanoseconds (t0 = " + minX + ")"); //We give the beginning time of the trace to the user with the X axis name 

			/* The name of the field is now displayed on it's axis
			if(fieldToDisplay.size() > 1) { //if multiple fields are displayed, show the legend else don't show it 
				chart.getLegend().setVisible(true);
			}
			else {
				chart.getLegend().setVisible(false);
			}
			*/
			
			IAxis yAxis = chart.getAxisSet().getYAxis(axisId);
//			yAxis.getTick().setFormat(new DecimalFormat("0.0000"));
			yAxis.getTitle().setText(getPrintableName(field)); //We set the name of the axis with the field's name
			yAxis.getTitle().setVisible(true); //And display it
			yAxis.getTick().setVisible(true);
			
			series.setYAxisId(axisId);//Give to the series the id for his axis
			
			// This part needs to run on the UI thread since it updates the chart SWT control
			Display.getDefault().asyncExec(new Runnable() {
				@Override
				public void run() {
					series.setXSeries(x); //give time data to the series
					series.setYSeries(y); //give field value data to the series

					if(y.length <= 10) { //if less than 10 value are displayed, change the way they're shown on the chart to be able to see them better
						series.setSymbolType(PlotSymbolType.CIRCLE);
					}
					else {
						series.setSymbolType(PlotSymbolType.NONE);
					}

					// Set the new range
					if (!xValues.isEmpty() && !(y.length == 0)) { //if data exist to be displayed
						if(xValues.size() == 1 && y.length == 1) { //if only one value
							chart.getAxisSet().getXAxis(0).setRange(new Range(minX/2, 2*maxX));
							chart.getAxisSet().getYAxis(axisId).setRange(new Range(minY/2, 2*maxY));
						}
						else { //multiple values
							chart.getAxisSet().getXAxis(0).setRange(new Range(0, x[x.length - 1]));
							if(minY == maxY) { //if the value of the field does not change during the time
								chart.getAxisSet().getYAxis(axisId).setRange(new Range(minY/2, maxY * 2));
							}
							else {
								chart.getAxisSet().getYAxis(axisId).setRange(new Range(minY, maxY));
							}
						}

					} else { //if empty
						chart.getAxisSet().getXAxis(0).setRange(new Range(0, 1));
						chart.getAxisSet().getYAxis(axisId).setRange(new Range(0, 1));
					}
					
					if(logScale) {
						yAxis.enableLogScale(true);
					}
					chart.getAxisSet().adjustRange();//we update the range
					
					chart.redraw(); //and redraw the chart
				}
			});
			
		}
		
		addInformationHover(chart, time0);//We add the listener for the hover tooltip
		
		if(fieldToDisplay.isEmpty()) { //if they are no field to show we do this
			chart.getAxisSet().getYAxis(0).getTitle().setVisible(false); //make it so that the nondisposable axis is less visible by making it so that his title is not displayed
		}
		return 0;
	}
	
	/**
	 * This function gives a ChartToPrint object with the values of the field act of the variable at variableQuark on the range of the VeritraceZoomView object.
	 * Use the {@link VeritraceAnalysisModule} to obtain data. 
	 * @param variableQuark quark of the variable we want the values of in the StateSystem
	 * @param act Name of the field in the state system
	 * @return ChartToPrint an object containing every important informations necessitated for printing the chart
	 */
	private ChartToPrint getValuesField(int variableQuark, String act){
		/* The max values for the axis */
		double maxY = -Double.MAX_VALUE;
		double minY = Double.MAX_VALUE;
		double maxX = -Double.MAX_VALUE;
		double minX = Double.MAX_VALUE;

		if(fTrace == null) {
			return null;
		}

		ITmfStateSystem ss = TmfStateSystemAnalysisModule.getStateSystem(fTrace, VeritraceAnalysisModule.ID);
		if (ss == null) {
			return null;
		}
		
		boolean complete = false;
		while(!complete) {//We wait for the state system to have finished being built
			complete = ss.waitUntilBuilt(BUILD_UPDATE_TIMEOUT);
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
		
		long startTime = getStartTime();
		long endTime = getEndTime();
		/* Make sure the times are correct */
		final long realStart = Math.max(startTime, ss.getStartTime()); //if the defined start time is lower than the trace starttime, the max will be chosen
		final long realEnd = Math.min(endTime, ss.getCurrentEndTime()); //same but with the endtime and the minimum
		if(realEnd <= realStart) {
			return null;
		}
		
		/* Retrieve analysis module needed to obtain data*/
		VeritraceAnalysisModule veritraceAnalysisModule = TmfTraceUtils.getAnalysisModuleOfClass(fTrace, VeritraceAnalysisModule.class, VeritraceAnalysisModule.ID);
		if (veritraceAnalysisModule == null) {
			return null;
		}
		
		int displayWidth = Display.getDefault().getBounds().width;
		long resolution = Long.max(1, (realEnd - realStart) / displayWidth); 
		/*We calculate the resolution so that printing the information gives relevant information while still being fast,
		 should not usually be a big problem on short trace but can be on big trace */
		long bucketLength = 2 * resolution;
		assert(bucketLength > 0);

		long queryStart = realStart;
		long queryEnd = queryStart + bucketLength;

		final ArrayList<Double> xValues = new ArrayList<Double>();
		final ArrayList<Double> yValues = new ArrayList<Double>();
		//We go from the start of the variable life (or the start of the zoom) to the end with a step of bucketlenght
		while (queryStart <= realEnd) {
			Map<Integer, Double> map = veritraceAnalysisModule.getValuesRange(queryStart, queryEnd, act); //We use the analysis module to obtain data on the wanted field (act)
			if (map.containsKey(variableQuark)) { //if the variable has data in the trace on the range
				Double significant_digits = map.get(variableQuark); 
				if (significant_digits != null) {
					xValues.add((double) queryStart);//The time to put on the X axis
					minX = Math.min(queryStart, minX);//We look if the min and max need to be updated
					maxX = Math.max(queryStart, maxX); 
					
					yValues.add(significant_digits.doubleValue()); //The data on the Y axis for the field (example 7.4 significant_digits)
					minY = Math.min(significant_digits.doubleValue(), minY); //Update the max and min on the Yaxis
					maxY = Math.max(significant_digits.doubleValue(), maxY); 
				}
			}
			queryStart = queryEnd;
			queryEnd += bucketLength; //We go forward from bucketLenght
		}
		
		ChartToPrint ret = new ChartToPrint(); //We create the new object used to return needed data for the chart
		ret.setMaxX(maxX);
		ret.setMaxY(maxY);
		ret.setMinX(minX);
		ret.setMinY(minY);
		ret.setxValues(xValues);
		ret.setyValues(yValues);
		return ret;
	}

	/**
	 * Create the hover tooltip for the chart showing information about the chart and the axis
	 * @param chart the chart where we add the mouse move listener
	 */
	private void addInformationHover(Chart chart, long t0) {
		
		if(mouseMoveListener != null) {
			MouseMoveListener listener = mouseMoveListener;
			chart.getPlotArea().removeMouseMoveListener(listener); //We remove the old mouse move listener
			mouseMoveListener = null;
		}
		
		MouseMoveListener listener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				IAxis axis = chart.getAxisSet().getXAxis(0);
				String strToolTip;
				if (axis != null){
					double x = axis.getDataCoordinate(e.x);
//					strToolTip = String.format("Time : %d", (long)x + t0);
					
//					long millis = TimeUnit.MILLISECONDS.convert((long)x+t0, TimeUnit.NANOSECONDS);
//					Date date = new Date(millis);
//					DateFormat dateFormat = new SimpleDateFormat("");					
//					strToolTip = String.format("Time : %s", dateFormat.format(date));
					
					strToolTip = String.format("Time : %s", formatTimeAbs((long)x+t0));
					
					for(String act : fieldToDisplay) { //Each field is added if it is displayed
						Integer idAxis = axisId.get(act);
						axis = chart.getAxisSet().getYAxis(idAxis); //We get the axis of the field
						if (axis != null){
							double y = axis.getDataCoordinate(e.y); //We get the value of the field for where the mouse is
							strToolTip += String.format("\n" + fieldsClean.get(act) + " : %.2f", y); //And add it to the tooltip
						}
					}
					chart.getPlotArea().setToolTipText(strToolTip); //We set the tooltip text containing the information about the chart on the mouse pointer
				}
			}
		};
		chart.getPlotArea().addMouseMoveListener(listener); //The listener is added to the chart where it will work on
		mouseMoveListener = listener;
	}
	private static final SimpleDateFormat TIME_FORMAT = new SimpleDateFormat("HH:mm:ss"); //$NON-NLS-1$
	private static final long MILLISEC_IN_NS = 1000000;
	private static final long SEC_IN_NS = 1000000000;
	/**
     * Formats time in ns to Calendar format: HH:MM:SS MMM.mmm.nnn
     * @param time
     *            The source time, in ns
     * @return the formatted time
     */
    public static String formatTimeAbs(long time) {
        // format time from nanoseconds to calendar time HH:MM:SS
        String stime = TIME_FORMAT.format(new Date(time / MILLISEC_IN_NS));
        StringBuilder str = new StringBuilder(stime);
        String ns = formatNs(time);
        if (!ns.isEmpty()) {
            str.append('.');
            /*
             * append the Milliseconds, MicroSeconds and NanoSeconds as specified in the
             * Resolution
             */
            str.append(ns);
        }
        return str.toString();
    }
	
	/**
     * Obtains the remainder fraction on unit Seconds of the entered value in
     * nanoseconds. e.g. input: 1241207054171080214 ns The number of fraction
     * seconds can be obtained by removing the last 9 digits: 1241207054 the
     * fractional portion of seconds, expressed in ns is: 171080214
     * @param srcTime
     *            The source time in ns
     * @return the formatted nanosec
     */
    public static String formatNs(long srcTime) {
        StringBuilder str = new StringBuilder();
        long ns = Math.abs(srcTime % SEC_IN_NS);
        String nanos = Long.toString(ns);
        str.append("000000000".substring(nanos.length())); //$NON-NLS-1$
        str.append(nanos);
        return str.substring(0, 9);
    }
	
	/**
	 * Transform a list in an array to be able to add it easily to a chart
	 * @param list the list to transform
	 * @return an array containing the data from list
	 */
	private double[] toArray(List<Double> list) {
		double[] d = new double[list.size()];
		for (int i = 0; i < list.size(); ++i) {
			d[i] = list.get(i);
		}

		return d;
	}
	
	/**
	 * Same working as toArray but it normalize at 0 the list in doing so
	 * @param list the list to transform in array
	 * @param min the minimum member of the list
	 * @return an array of double
	 */
	private double[] toArrayNormalized(ArrayList<Double> list, double min) {
		double[] d = new double[list.size()];
		for (int i = 0; i < list.size(); ++i) {
			d[i] = list.get(i) - min;
		}

		return d;
	}
	
	/**
	 * Transform the list in absolute value
	 * @param list the list to process
	 * @return a new list with the processed value
	 */
	private ArrayList<Double> getAbsoluteArray(ArrayList<Double> list){
		ArrayList<Double> result = new ArrayList<>();
		for(Double act : list) {
			result.add(Math.abs(act));
		}
		return result;
	}

	

	/**
	 * Getter for the original field that is to be shown on the chart (default: significant_digits)
	 */
	public static String getOriginalField() {
		return originalField;
	}

	/**
	 * @return The name of the variable this chart is representing
	 */
	public String getVariableName() {
		return variableName;
	}

	public void setFocus() {
		chart.setFocus();
	}
	
	/**
	 * Redraw the chart
	 */
	public void redraw() {
		chart.redraw();
	}

	/**
	 * Called by the {@link VeritraceZoomView} when it receive a TmfSelectionRangeUpdatedSignal
	 * The caller ensure that startTime <= endTime.
	 * @param startTime the start time
	 * @param endTime the endtime
	 */
	public void selectionRange(long startTime, long endTime) {
		if(rangeToggled) { //If we accept zoom
			setStartTime(startTime);
			setEndTime(endTime);
			populateChart();
		}
	}
}
