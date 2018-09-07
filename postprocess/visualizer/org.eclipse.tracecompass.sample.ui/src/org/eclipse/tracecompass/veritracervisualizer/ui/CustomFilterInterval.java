package org.eclipse.tracecompass.veritracervisualizer.ui;

/**
 * Represent an interval of value for min and max significant digits to filter TimeEvent on the Global View
 * @author Damien Thenot
 */
public class CustomFilterInterval {
	private double filterUp; //The up bound for value to be shown
	private double filterDown; //The down limit

	public CustomFilterInterval(double downOrigin, double upOrigin) {
		this.filterDown = downOrigin;
		this.filterUp = upOrigin;
	}

	public double getFilterUp() {
		return filterUp;
	}

	public void setFilterUp(double filterUp) {
		this.filterUp = filterUp;
	}
	
	public double getFilterDown() {
		return filterDown;
	}

	public void setFilterDown(double filterDown) {
		this.filterDown = filterDown;
	}

	/**
	 * Return true if s is in the interval filterDown <= s <= filterUp else return false.
	 * @param s the Double value to inspect
	 * @return boolean
	 */
	public boolean evaluate(Double s) {
		double sd = s.doubleValue();
		if(filterDown <= sd && sd <= filterUp) {
			return true;
		}
		return false;
	}

	
	
	
}
