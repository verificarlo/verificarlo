##############################################################################\
#                                                                           #\
#  This file is part of the Verificarlo project,                            #\
#  under the Apache License v2.0 with LLVM Exceptions.                      #\
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 #\
#  See https://llvm.org/LICENSE.txt for license information.                #\
#                                                                           #\
#                                                                           #\
#  Copyright (c) 2015                                                       #\
#     Universite de Versailles St-Quentin-en-Yvelines                       #\
#     CMLA, Ecole Normale Superieure de Cachan                              #\
#                                                                           #\
#  Copyright (c) 2018                                                       #\
#     Universite de Versailles St-Quentin-en-Yvelines                       #\
#                                                                           #\
#  Copyright (c) 2019-2021                                                  #\
#     Verificarlo Contributors                                              #\
#                                                                           #\
#############################################################################

# Manage the view comparing results of deerministic backends over runs.
# At its creation, a DeterministicCompare object will create all the needed
# Bokeh widgets and plots, setup the callback functions (either server side or
# client side), initialize widgets selection, and from this selection generate
# the first plots. Then, when callback functions are triggered, widgets
# selections are updated, and plots are re-generated with the newly selected
# data.

from math import nan

import helper
import plot
from bokeh.models import (
    CheckboxGroup,
    ColumnDataSource,
    CustomJS,
    Select,
    TextInput,
)
from bokeh.plotting import figure

##########################################################################


class DeterministicCompare:
    # Plots update function

    def update_plots(self):
        if self.data.empty:
            # Initialize empty dict and return
            dict = {"value_x": [], "value": [], "ieee": [], "custom_colors": []}
            self.source.data = dict

            return

        # Select all data matching current test/var/backend

        runs = self.data.loc[
            [self.widgets["select_deterministic_test"].value],
            self.widgets["select_deterministic_var"].value,
            self.widgets["select_deterministic_backend"].value,
        ]

        runs = runs.sort_values(by=["timestamp"])

        timestamps = runs["timestamp"]
        x_series, x_metadata = helper.gen_x_series(
            self.metadata, timestamps.sort_values(), self.current_n_runs
        )

        # Update source

        dict = runs.to_dict("series")
        dict["value_x"] = x_series

        # Add metadata (for tooltip)
        dict.update(x_metadata)

        # Select the last n runs only
        n = self.current_n_runs
        dict = {key: value[-n:] for key, value in dict.items()}

        # Generate color series for display of failed checks
        dict["custom_colors"] = [True] * len(dict["check"])
        for i in range(len(dict["check"])):
            dict["custom_colors"][i] = "#1f77b4" if dict["check"][i] else "#cc2b2b"

        # Filter outliers if the box is checked
        if len(self.widgets["outliers_filtering_deterministic"].active) > 0:
            outliers = helper.detect_outliers(dict["value"])
            dict["value"] = helper.remove_outliers(dict["value"], outliers)
            dict["value_x"] = helper.remove_outliers(dict["%value_x"], outliers)

        # Series to display IEEE results. This is different than reference_value,
        # because the latter has to be 0 when no reference run has been done
        # (because the value will be shown in the tooltip)
        dict["ieee"] = [0] * len(dict["value"])

        for i in range(len(dict["value"])):
            if dict["check"][i]:
                dict["ieee"][i] = nan
            else:
                dict["ieee"][i] = dict["reference_value"][i]

        self.source.data = dict

        # Update x axis

        helper.reset_x_range(self.plots["comparison_plot"], self.source.data["value_x"])

        # Widgets' callback functions

    def update_test(self, attrname, old, new):
        # If the value is updated by the CustomJS,
        # self.widgets["select_deterministic_var"].value won't be updated, so we
        # have to look for that case and assign it manually.

        # "new" should be a list when updated by CustomJS
        if isinstance(new, list):
            # If filtering removed all options, we might have an empty list
            # (in this case, we just skip the callback and do nothing)
            if len(new) > 0:
                new = new[0]
            else:
                return

        # Speciel case when nex is an empty string, triggered by no data
        # to show
        if len(new) == 0:
            return

        if new != self.widgets["select_deterministic_test"].value:
            # The callback will be triggered again with the updated value
            self.widgets["select_deterministic_test"].value = new
            return

        # New list of available vars
        self.vars = (
            self.data.loc[new]
            .index.get_level_values("variable")
            .drop_duplicates()
            .tolist()
        )
        self.widgets["select_deterministic_var"].options = self.vars

        # Reset var selection if old one is not available in new vars
        if self.widgets["select_deterministic_var"].value not in self.vars:
            self.widgets["select_deterministic_var"].value = self.vars[0]
            # The update_var callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_var("", "", self.widgets["select_deterministic_var"].value)

    def update_var(self, attrname, old, new):
        # If the value is updated by the CustomJS,
        # self.widgets["select_deterministic_var"].value won't be updated, so we
        # have to look for that case and assign it manually.

        # new should be a list when updated by CustomJS
        if isinstance(new, list):
            new = new[0]

        if new != self.widgets["select_deterministic_var"].value:
            # The callback will be triggered again with the updated value
            self.widgets["select_deterministic_var"].value = new
            return

        # New list of available backends
        self.backends = (
            self.data.loc[
                self.widgets["select_deterministic_test"].value,
                self.widgets["select_deterministic_var"].value,
            ]
            .index.get_level_values("vfc_backend")
            .drop_duplicates()
            .tolist()
        )
        self.widgets["select_deterministic_backend"].options = self.backends

        # Reset backend selection if old one is not available in new backends
        if self.widgets["select_deterministic_backend"].value not in self.backends:
            self.widgets["select_deterministic_backend"].value = self.backends[0]
            # The update_backend callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_backend(
                "", "", self.widgets["select_deterministic_backend"].value
            )

    def update_backend(self, attrname, old, new):
        # Simply update plots, since no other data is affected
        self.update_plots()

    def update_n_runs(self, attrname, old, new):
        # Simply update runs selection (value and string display)
        self.widgets["select_n_deterministic_runs"].value = new
        self.current_n_runs = self.n_runs_dict[
            self.widgets["select_n_deterministic_runs"].value
        ]

        self.update_plots()

    def update_outliers_filtering(self, attrname, old, new):
        self.update_plots()

        # Bokeh setup functions

    def setup_plots(self):
        tools = "pan, wheel_zoom, xwheel_zoom, ywheel_zoom, reset, save"

        # Backend Vs Reference comparison plot
        self.plots["comparison_plot"] = figure(
            name="comparison_plot",
            title="Variable with deterministic backend over runs",
            plot_width=900,
            plot_height=400,
            x_range=[""],
            tools=tools,
            sizing_mode="scale_width",
        )

        comparison_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("Backend value", "@value{%0.18e}"),
            ("Reference value", "@reference_value{%0.18e}"),
            ("Accuracy target", "@accuracy_threshold"),
        ]
        comparison_tooltips_formatters = {
            "@value": "printf",
            "@reference_value": "printf",
        }

        js_tap_callback = 'changeView("checks");'

        plot.fill_dotplot(
            self.plots["comparison_plot"],
            self.source,
            data_field="value",
            tooltips=comparison_tooltips,
            tooltips_formatters=comparison_tooltips_formatters,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.checks_callback,
            lines=True,
            second_series="ieee",  # Will be used to display the IEEE results
            legend="Backend value",
            second_legend="IEEE reference value",
            custom_colors="custom_colors",
        )

        self.doc.add_root(self.plots["comparison_plot"])

    def setup_widgets(self):
        # Initial selection

        # Test/var/backend combination (we select all first elements at init)
        if not self.data.empty:
            self.tests = (
                self.data.index.get_level_values("test").drop_duplicates().tolist()
            )

            self.vars = (
                self.data.loc[self.tests[0]]
                .index.get_level_values("variable")
                .drop_duplicates()
                .tolist()
            )

            self.backends = (
                self.data.loc[self.tests[0], self.vars[0]]
                .index.get_level_values("vfc_backend")
                .drop_duplicates()
                .tolist()
            )

        else:
            self.tests = ["None"]
            self.vars = ["None"]
            self.backends = ["None"]

        # Custom JS callback that will be used client side to filter selections
        filter_callback_js = """
        selector.options = options.filter(e => e.includes(cb_obj.value));
        """

        # Test selector widget

        # Number of runs to display
        # The dict structure allows us to get int value from the display string
        # in O(1)
        self.n_runs_dict = {
            "Last 3 runs": 3,
            "Last 5 runs": 5,
            "Last 10 runs": 10,
            "All runs": 0,
        }

        # Contains all options strings
        n_runs_display = list(self.n_runs_dict.keys())

        # Will be used when updating plots (contains actual number to diplay)
        self.current_n_runs = self.n_runs_dict[n_runs_display[1]]

        # Selector widget
        self.widgets["select_deterministic_test"] = Select(
            name="select_deterministic_test",
            title="Test :",
            value=self.tests[0],
            options=self.tests,
        )
        self.doc.add_root(self.widgets["select_deterministic_test"])
        self.widgets["select_deterministic_test"].on_change("value", self.update_test)
        self.widgets["select_deterministic_test"].on_change("options", self.update_test)

        # Filter widget
        self.widgets["deterministic_test_filter"] = TextInput(
            name="deterministic_test_filter", title="Tests filter:"
        )
        self.widgets["deterministic_test_filter"].js_on_change(
            "value",
            CustomJS(
                args=dict(
                    options=self.tests,
                    selector=self.widgets["select_deterministic_test"],
                ),
                code=filter_callback_js,
            ),
        )
        self.doc.add_root(self.widgets["deterministic_test_filter"])

        # Number of runs to display

        self.widgets["select_n_deterministic_runs"] = Select(
            name="select_n_deterministic_runs",
            title="Display :",
            value=n_runs_display[1],
            options=n_runs_display,
        )
        self.doc.add_root(self.widgets["select_n_deterministic_runs"])
        self.widgets["select_n_deterministic_runs"].on_change(
            "value", self.update_n_runs
        )

        # Variable selector widget

        self.widgets["select_deterministic_var"] = Select(
            name="select_deterministic_var",
            title="Variable :",
            value=self.vars[0],
            options=self.vars,
        )
        self.doc.add_root(self.widgets["select_deterministic_var"])
        self.widgets["select_deterministic_var"].on_change("value", self.update_var)
        self.widgets["select_deterministic_var"].on_change("options", self.update_var)

        # Backend selector widget

        self.widgets["select_deterministic_backend"] = Select(
            name="select_deterministic_backend",
            title="Verificarlo backend (deterministic) :",
            value=self.backends[0],
            options=self.backends,
        )
        self.doc.add_root(self.widgets["select_deterministic_backend"])
        self.widgets["select_deterministic_backend"].on_change(
            "value", self.update_backend
        )

        # Outliers filtering checkbox

        self.widgets["outliers_filtering_deterministic"] = CheckboxGroup(
            name="outliers_filtering_deterministic",
            labels=["Filter outliers"],
            active=[],
        )
        self.doc.add_root(self.widgets["outliers_filtering_deterministic"])
        self.widgets["outliers_filtering_deterministic"].on_change(
            "active", self.update_outliers_filtering
        )

        # Communication methods
        # (to send/receive messages to/from master)

    def checks_callback(self, attrname, old, new):
        """
        Callback to change view to "Checks" view when plot element is clicked
        """

        # In case we just unselected everything on the plot, then do nothing
        if not new:
            return

        # But if an element has been selected, go to the corresponding run
        index = new[-1]
        run_name = self.source.data["value_x"][index]

        self.master.go_to_checks(run_name)

    def change_repo(self, new_data, new_metadata):
        """
        When received, update data and metadata with the new repo, and update
        everything
        """

        self.data = new_data
        self.metadata = new_metadata

        # Update widgets(and automatically trigger plot updates)
        self.tests = self.data.index.get_level_values("test").drop_duplicates().tolist()

        if len(self.tests) != 0:
            self.vars = (
                self.data.loc[self.tests[0]]
                .index.get_level_values("variable")
                .drop_duplicates()
                .tolist()
            )
        else:
            self.vars = []

        if len(self.vars) != 0:
            self.backends = (
                self.data.loc[self.tests[0], self.vars[0]]
                .index.get_level_values("vfc_backend")
                .drop_duplicates()
                .tolist()
            )
        else:
            self.backends = []

        self.widgets["select_deterministic_test"].options = self.tests
        if len(self.tests) != 0:
            self.widgets["select_deterministic_test"].value = self.tests[0]
        else:
            self.widgets["select_deterministic_test"].value = ""

        # If changing repo doesn't affect the selection, trigger the callback
        # manually
        old = self.widgets["select_deterministic_backend"].value
        if self.widgets["select_deterministic_backend"].value == old:
            self.update_plots()

    # Constructor

    def __init__(self, master, doc, data, metadata):
        """
        Here are the most important attributes of the CompareRuns class

        master : reference to the ViewMaster class
        doc : an object provided by Bokeh to add elements to the HTML document
        data : pandas dataframe containing all the tests data (only for
        deterministic backends)
        metadata : pandas dataframe containing all the tests metadata

        sources : ColumnDataSource object provided by Bokeh, contains current
        data for the plots (inside the .data attribute)
        plots : dictionary of Bokeh plots
        widgets : dictionary of Bokeh widgets
        """

        self.master = master

        self.doc = doc
        self.data = data
        self.metadata = metadata

        self.source = ColumnDataSource(data={})

        self.plots = {}
        self.widgets = {}

        # Setup Bokeh objects
        self.setup_plots()
        self.setup_widgets()

        # At this point, everything should have been initialized, so we can
        # show the plots for the first time
        self.update_plots()
