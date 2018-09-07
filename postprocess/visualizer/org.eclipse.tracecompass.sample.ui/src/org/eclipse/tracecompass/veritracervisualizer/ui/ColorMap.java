package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.swt.graphics.RGB;

/**
 * Class allowing the link between a value and it's color on the TimeGraph in the GlobalView
 * @author Damien Thenot
 *
 */
public class ColorMap {

	private RGB colorBegin, colorMid, colorEnd; //The different colors representing the color gradient
	private double max, min; //The max and min of the possible value we need to obtain the color from
	
	public ColorMap(RGB color1, RGB color2, RGB color3, double min, double max) {
		this.colorBegin = color1;
		this.colorMid = color2;
		this.colorEnd = color3;
		this.min = min;
		this.max = max;
	}
	
	
	public ColorMap(double min, double max) {
		this.colorBegin = new RGB(255, 0, 0);
		this.colorMid = new RGB(255, 255, 0);
		this.colorEnd = new RGB(0, 255, 0);
		this.min = min;
		this.max = max;
	}
	
	/**
	 * Return a color for var depending of it's value comprised between min and max of this ColorMap object
	 * @param var a double between min and max
	 * @return a RGB color on the gradient according to the value of var
	 */
	public RGB getColor(double var) {
		double p = normalize(min, max, var);
		
		if(p <= 0.5) {//if the normalized value is in the first part of the gradient
			//usually from red to yellow
			double ptmp = normalize(0F, 0.5F, p); // We re-normalize to have a double between 0 and 1 to be able to calculate the place of var on the gradient
			int rtmp =  (int) (colorMid.red * ptmp + colorBegin.red * (1 - ptmp));
			int gtmp = (int) (colorMid.green * ptmp + colorBegin.green * (1 - ptmp));
			int btmp = (int) (colorMid.blue * ptmp + colorBegin.blue * (1 - ptmp));
			return new RGB(rtmp, gtmp, btmp);
		}
		else {
			//usually from yellow to green
			double ptmp = normalize(0.5F, 1F, p);
			int rtmp =  (int) (colorEnd.red * ptmp + colorMid.red * (1 - ptmp));
			int gtmp = (int) (colorEnd.green * ptmp + colorMid.green * (1 - ptmp));
			int btmp = (int) (colorEnd.blue * ptmp + colorMid.blue * (1 - ptmp));
			return new RGB(rtmp, gtmp, btmp);
		}
		
	}

	/**
	 * Normalize value with min2 and max2 as min and max
	 * @param min2 the minimum value can be
	 * @param max2 the maximum value can be
	 * @param value the value to normalize
	 * @return the normalize value, it's a double between 0 and 1
	 */
	public static double normalize(double min2, double max2, double value) {
		return (value - min2) / (max2 - min2);
	}
}
