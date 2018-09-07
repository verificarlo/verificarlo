package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.Set;

import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.tracecompass.tmf.core.signal.TmfSelectionRangeUpdatedSignal;
import org.eclipse.tracecompass.tmf.core.timestamp.TmfTimestamp;
import org.eclipse.tracecompass.tmf.ui.views.TmfView;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.ICustomPaintListener;
import org.swtchart.IPlotArea;

/**
 * Provide the zoom functionalities on the Zoom View
 * @author Damien Thenot
 */
public class VeritraceZoomViewZoomProvider implements ICustomPaintListener, MouseListener, MouseMoveListener{
	private Point begin = new Point(0, 0), end = new Point(0, 0); //begin and ending point of the selection 
	private TmfView view; //The view creating this provider
	private Chart chart; //The chart this provider is giving zoom functionalities
	private long time0; //The t0 of the time of the chart
	private boolean isUpdate; 

	public VeritraceZoomViewZoomProvider(TmfView view, Chart chart, Set<String> fieldToDisplay) {
		this.view = view;
		this.chart = chart;

		chart.getPlotArea().addMouseListener(this); //We define this listeners implementation as the chart listener to handle the mouse
		chart.getPlotArea().addMouseMoveListener(this);
		((IPlotArea) chart.getPlotArea()).addCustomPaintListener(this);
	}
	
	public void setTime0(long time0) {
		this.time0 = time0;
	}

	@Override
	public void paintControl(PaintEvent e) {
		if (isUpdate && !begin.equals(end)) {
			int startX = begin.x;
			int endX = end.x;
			//            e.gc.setBackground(TmfXYChartViewer.getDisplay().getSystemColor(SWT.COLOR_TITLE_INACTIVE_BACKGROUND));
			if (begin.x < end.x) {
				e.gc.fillRectangle(startX, 0, endX - startX, e.height);
			} else {
				e.gc.fillRectangle(endX, 0, startX - endX, e.height);
			}
			e.gc.drawLine(startX, 0, startX, e.height);
			e.gc.drawLine(endX, 0, endX, e.height);
		}
	}

	/**
	 * Action for when the mouse move
	 * @param e
	 */
	@Override
	public void mouseMove(MouseEvent e) {
		end.x = e.x;
		end.y = e.y;
		chart.redraw();
	}

	@Override
	public void mouseDoubleClick(MouseEvent e) {
	}

	/**
	 * Action for when the mouse button is pushed down
	 * @param e
	 */
	@Override
	public void mouseDown(MouseEvent e) {
		if(e.button == 1) { //Left mouse button
			begin.x = e.x;
			begin.y = e.y;
			isUpdate = true;
		}
	}

	/**
	 * Action for when the button is released
	 * @param e
	 */
	@Override
	public void mouseUp(MouseEvent e) {
		if(e.button == 1) { //Left mouse button
			IAxis xAxis = chart.getAxisSet().getXAxis(0);
			if(xAxis != null) {
				long beginTs = (long) (xAxis.getDataCoordinate(begin.x) + time0);
				long endTs = (long) (xAxis.getDataCoordinate(e.x) + time0);
				view.broadcast(new TmfSelectionRangeUpdatedSignal(this, TmfTimestamp.fromNanos(beginTs), TmfTimestamp.fromNanos(endTs)));
			}
			if(isUpdate) {
				chart.redraw();
			}
			isUpdate = false;
		}
	}

	@Override
	public boolean drawBehindSeries() {
		return true;
	}

	/**
	 * Called to cleanly dispose of this ZoomProvider
	 */
	public void deregister() {
		chart.getPlotArea().removeMouseListener(this);
		chart.getPlotArea().removeMouseMoveListener(this);
		((IPlotArea) chart.getPlotArea()).removeCustomPaintListener(this);
	}
}
