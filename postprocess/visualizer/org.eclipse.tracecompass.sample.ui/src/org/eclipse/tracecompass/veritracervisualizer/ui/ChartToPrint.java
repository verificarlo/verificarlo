package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.ArrayList;

/**
 * Class allowing to contain everything needed to draw a SWT Chart
 * @author Damien Thenot
 *
 */
public class ChartToPrint {
	
	private double minX, minY, maxX, maxY; //The limit of both axis
	private ArrayList<Double> xValues, yValues; //Array of the point on the axis
	
	public double getMinX() {
		return minX;
	}
	
	public void setMinX(double minX) {
		this.minX = minX;
	}
	
	public double getMinY() {
		return minY;
	}
	
	public void setMinY(double minY) {
		this.minY = minY;
	}
	
	public double getMaxX() {
		return maxX;
	}
	
	public void setMaxX(double maxX) {
		this.maxX = maxX;
	}
	
	public double getMaxY() {
		return maxY;
	}
	
	public void setMaxY(double maxY) {
		this.maxY = maxY;
	}
	
	public ArrayList<Double> getxValues() {
		return xValues;
	}
	
	public void setxValues(ArrayList<Double> xValues) {
		this.xValues = xValues;
	}
	
	public ArrayList<Double> getyValues() {
		return yValues;
	}
	
	public void setyValues(ArrayList<Double> yValues) {
		this.yValues = yValues;
	}
}
