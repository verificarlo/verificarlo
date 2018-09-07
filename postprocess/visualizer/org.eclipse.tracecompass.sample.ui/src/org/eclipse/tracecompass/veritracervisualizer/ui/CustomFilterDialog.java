package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

/**
 * Dialog asking the user to set a limit for floating-point operation problem.
 * Every variable having at least one event dipping under the limit will be shown and every event not under this limit will be hidden,
 * it allows to easily notice problematic floating-point period.
 * @author Damien Thenot
 *
 */
public class CustomFilterDialog extends Dialog{

	private Text valueUp; //the text widget containing the user chosen limit and up value for the interval
	private double limit; //the limit under which entry and events are seen as problematic
	private CustomFilterInterval fFilter;//The interval in which events needs to be to be considered

	protected CustomFilterDialog(Shell parentShell, CustomFilterInterval filter) {
		super(parentShell);
		limit = Double.MAX_VALUE;
		fFilter = filter;
	}

	/**
	 * Create every object which are part of this dialog window.
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite container = (Composite) super.createDialogArea(parent);

		GridData dataValue = new GridData();
		dataValue.grabExcessHorizontalSpace = true;
		dataValue.horizontalAlignment = GridData.FILL;

		///Up
		Label labelUp = new Label(container, SWT.NONE);
		labelUp.setText("Limit of interest for s, if s < limit then it is displayed : "); //Label on the left of the text box

		valueUp = new Text(container, SWT.BORDER); //Text box used to communicate with the user
		valueUp.setText(Double.toString(fFilter.getFilterUp()));
		valueUp.setLayoutData(dataValue);
		valueUp.addListener(SWT.Modify, new Listener() { //Action to perform when valueUp is changed
			@Override
			public void handleEvent(Event event) {
				double fValue = Double.MAX_VALUE;
				try {
					fValue = Double.parseDouble(valueUp.getText());
				}
				catch(Exception e){

				}
				limit = fValue;
				fFilter.setFilterUp(fValue);
			}
		});

		Button resetUp = new Button(container, SWT.NONE); //Button to reset the limit to MAX_VALUE
		resetUp.setText("Reset");
		resetUp.addSelectionListener(new SelectionListener() {

			@Override
			public void widgetSelected(SelectionEvent e) {
				limit = Double.MAX_VALUE;
				fFilter.setFilterUp(Double.MAX_VALUE);
				valueUp.setText(Double.toString(fFilter.getFilterUp()));
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e) {}			
		});

		return container;
	}

	public double getLimit() {
		return limit;
	}

	@Override
	protected boolean isResizable() {
		return false;
	}

	@Override
	protected void configureShell(Shell newShell) {
		super.configureShell(newShell);
		newShell.setText("Filter by significant digits : "); //Name of the pop-up window
	}

	@Override
	protected Point getInitialSize() {
		return new Point(450, 300);
	}
}