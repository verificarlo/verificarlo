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

# Manage the view comparing a variable over different runs
# At its creation, a CompareRuns object will create all the needed Bokeh widgets
# and plots, setup the callback functions (either server side or client side),
# initialize widgets selection, and from this selection generate the first plots.
# Then, when callback functions are triggered, widgets selections are updated,
# and plots are re-generated with the newly selected data.


import helper
import plot
from bokeh.models import (
    CheckboxGroup,
    ColumnDataSource,
    CustomJS,
    Panel,
    Select,
    Tabs,
    TextInput,
)

##########################################################################


class CompareRuns:
    # Plots update function

    def update_plots(self):
        if self.data.empty:
            # Initialize empty dicts and return

            for stat in ["sigma", "s10", "s2"]:
                dict = {
                    "%s_x" % stat: [],
                    "is_git_commit": [],
                    "date": [],
                    "hash": [],
                    "author": [],
                    "message": [],
                    stat: [],
                    "nsamples": [],
                    "accuracy_threshold": [],
                    "custom_colors": [],
                }

                if stat == "s10" or stat == "s2":
                    dict["%s_lower_bound" % stat] = []

                self.sources["%s_source" % stat].data = dict

            # Boxplot dict
            dict = {
                "is_git_commit": [],
                "date": [],
                "hash": [],
                "author": [],
                "message": [],
                "x": [],
                "min": [],
                "quantile25": [],
                "quantile50": [],
                "quantile75": [],
                "max": [],
                "mu": [],
                "pvalue": [],
                "nsamples": [],
                "custom_colors": [],
            }

            self.sources["boxplot_source"].data = dict

            return

        # Select all data matching current test/var/backend

        runs = self.data.loc[
            [self.widgets["select_test"].value],
            self.widgets["select_var"].value,
            self.widgets["select_backend"].value,
        ]

        runs = runs.sort_values(by=["timestamp"])

        timestamps = runs["timestamp"]
        x_series, x_metadata = helper.gen_x_series(
            self.metadata, timestamps.sort_values(), self.current_n_runs
        )

        # Update source

        main_dict = runs.to_dict("series")
        main_dict["x"] = x_series

        # Add metadata (for tooltip)
        main_dict.update(x_metadata)

        # Select the last n runs only
        n = self.current_n_runs
        main_dict = {key: value[-n:] for key, value in main_dict.items()}

        # Generate color series for display of failed checks
        custom_colors = [True] * len(main_dict["check"])
        for i in range(len(main_dict["check"])):
            custom_colors[i] = "#1f77b4" if main_dict["check"][i] else "#cc2b2b"

        # Generate ColumnDataSources for the 3 dotplots
        for stat in ["sigma", "s10", "s2"]:
            dict = {
                "%s_x" % stat: main_dict["x"],
                "is_git_commit": main_dict["is_git_commit"],
                "date": main_dict["date"],
                "hash": main_dict["hash"],
                "author": main_dict["author"],
                "message": main_dict["message"],
                stat: main_dict[stat],
                "nsamples": main_dict["nsamples"],
                "accuracy_threshold": main_dict["accuracy_threshold"],
                "custom_colors": custom_colors,
            }

            if stat == "s10" or stat == "s2":
                dict["%s_lower_bound" % stat] = main_dict["%s_lower_bound" % stat]

            # Filter outliers if the box is checked
            if len(self.widgets["outliers_filtering_compare"].active) > 0:
                outliers = helper.detect_outliers(dict[stat])
                dict[stat] = helper.remove_outliers(dict[stat], outliers)
                dict["%s_x" % stat] = helper.remove_outliers(
                    dict["%s_x" % stat], outliers
                )

            # Assign ColumnDataSource
            self.sources["%s_source" % stat].data = dict

        # Generate ColumnDataSource for the boxplot
        dict = {
            "is_git_commit": main_dict["is_git_commit"],
            "date": main_dict["date"],
            "hash": main_dict["hash"],
            "author": main_dict["author"],
            "message": main_dict["message"],
            "x": main_dict["x"],
            "min": main_dict["min"],
            "quantile25": main_dict["quantile25"],
            "quantile50": main_dict["quantile50"],
            "quantile75": main_dict["quantile75"],
            "max": main_dict["max"],
            "mu": main_dict["mu"],
            "pvalue": main_dict["pvalue"],
            "nsamples": main_dict["nsamples"],
            "custom_colors": custom_colors,
        }

        self.sources["boxplot_source"].data = dict

        # Update x axis

        helper.reset_x_range(
            self.plots["boxplot"], self.sources["boxplot_source"].data["x"]
        )
        helper.reset_x_range(
            self.plots["sigma_plot"], self.sources["sigma_source"].data["sigma_x"]
        )
        helper.reset_x_range(
            self.plots["s10_plot"], self.sources["s10_source"].data["s10_x"]
        )
        helper.reset_x_range(
            self.plots["s2_plot"], self.sources["s2_source"].data["s2_x"]
        )

        # Widgets' callback functions

    def update_test(self, attrname, old, new):
        # If the value is updated by the CustomJS, self.widgets["select_var"].value
        # won't be updated, so we have to look for that case and assign it
        # manually

        # "new" should be a list when updated by CustomJS
        if isinstance(new, list):
            # If filtering removed all options, we might have an empty list
            # (in this case, we just skip the callback and do nothing)
            if len(new) > 0:
                new = new[0]
            else:
                return

        if new != self.widgets["select_test"].value:
            # The callback will be triggered again with the updated value
            self.widgets["select_test"].value = new
            return

        # New list of available vars
        self.vars = (
            self.data.loc[new]
            .index.get_level_values("variable")
            .drop_duplicates()
            .tolist()
        )
        self.widgets["select_var"].options = self.vars

        # Reset var selection if old one is not available in new vars
        if self.widgets["select_var"].value not in self.vars:
            self.widgets["select_var"].value = self.vars[0]
            # The update_var callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_var("", "", self.widgets["select_var"].value)

    def update_var(self, attrname, old, new):
        # If the value is updated by the CustomJS, self.widgets["select_var"].value
        # won't be updated, so we have to look for that case and assign it
        # manually

        # new should be a list when updated by CustomJS
        if isinstance(new, list):
            new = new[0]

        if new != self.widgets["select_var"].value:
            # The callback will be triggered again with the updated value
            self.widgets["select_var"].value = new
            return

        # New list of available backends
        self.backends = (
            self.data.loc[
                self.widgets["select_test"].value, self.widgets["select_var"].value
            ]
            .index.get_level_values("vfc_backend")
            .drop_duplicates()
            .tolist()
        )
        self.widgets["select_backend"].options = self.backends

        # Reset backend selection if old one is not available in new backends
        if self.widgets["select_backend"].value not in self.backends:
            self.widgets["select_backend"].value = self.backends[0]
            # The update_backend callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_backend("", "", self.widgets["select_backend"].value)

    def update_backend(self, attrname, old, new):
        # Simply update plots, since no other data is affected
        self.update_plots()

    def update_n_runs(self, attrname, old, new):
        # Simply update runs selection (value and string display)
        self.widgets["select_n_runs"].value = new
        self.current_n_runs = self.n_runs_dict[self.widgets["select_n_runs"].value]

        self.update_plots()

    def update_outliers_filtering(self, attrname, old, new):
        self.update_plots()

        # Bokeh setup functions

    def setup_plots(self):
        tools = "pan, wheel_zoom, xwheel_zoom, ywheel_zoom, reset, save"

        # Custom JS callback that will be used when tapping on a run
        # Only switches the view, a server callback is required to update plots
        js_tap_callback = 'changeView("inspect-runs");'

        # Box plot
        self.plots["boxplot"] = figure(
            name="boxplot",
            title="Variable distribution over runs",
            plot_width=900,
            plot_height=400,
            x_range=[""],
            tools=tools,
            sizing_mode="scale_width",
        )

        box_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("Min", "@min{%0.18e}"),
            ("Max", "@max{%0.18e}"),
            ("1st quartile", "@quantile25{%0.18e}"),
            ("Median", "@quantile50{%0.18e}"),
            ("3rd quartile", "@quantile75{%0.18e}"),
            ("μ", "@mu{%0.18e}"),
            ("p-value", "@pvalue"),
            ("Number of samples", "@nsamples"),
        ]
        box_tooltips_formatters = {
            "@min": "printf",
            "@max": "printf",
            "@quantile25": "printf",
            "@quantile50": "printf",
            "@quantile75": "printf",
            "@mu": "printf",
        }

        plot.fill_boxplot(
            self.plots["boxplot"],
            self.sources["boxplot_source"],
            tooltips=box_tooltips,
            tooltips_formatters=box_tooltips_formatters,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_boxplot,
            custom_colors="custom_colors",
        )
        self.doc.add_root(self.plots["boxplot"])

        # Sigma plot (bar plot)
        self.plots["sigma_plot"] = figure(
            name="sigma_plot",
            title="Standard deviation σ over runs",
            plot_width=900,
            plot_height=400,
            x_range=[""],
            tools=tools,
            sizing_mode="scale_width",
        )

        sigma_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("σ", "@sigma"),
            ("Number of samples", "@nsamples"),
            ("Check's accuracy target", "@accuracy_threshold"),
        ]

        plot.fill_dotplot(
            self.plots["sigma_plot"],
            self.sources["sigma_source"],
            "sigma",
            tooltips=sigma_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_sigma,
            lines=True,
            custom_colors="custom_colors",
        )
        self.doc.add_root(self.plots["sigma_plot"])

        # s plot (bar plot with 2 tabs)
        self.plots["s10_plot"] = figure(
            name="s10_plot",
            title="Significant digits s over runs",
            plot_width=900,
            plot_height=400,
            x_range=[""],
            tools=tools,
            sizing_mode="scale_width",
        )

        s10_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("s", "@s10"),
            ("s lower bound", "@s10_lower_bound"),
            ("Number of samples", "@nsamples"),
            ("Check's accuracy target", "@accuracy_threshold"),
        ]

        plot.fill_dotplot(
            self.plots["s10_plot"],
            self.sources["s10_source"],
            "s10",
            tooltips=s10_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_s10,
            lines=True,
            lower_bound=True,
            custom_colors="custom_colors",
        )
        s10_tab = Panel(child=self.plots["s10_plot"], title="Base 10")

        self.plots["s2_plot"] = figure(
            name="s2_plot",
            title="Significant digits s over runs",
            plot_width=900,
            plot_height=400,
            x_range=[""],
            tools=tools,
            sizing_mode="scale_width",
        )

        s2_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("s", "@s2"),
            ("s lower bound", "@s2_lower_bound"),
            ("Number of samples", "@nsamples"),
        ]

        plot.fill_dotplot(
            self.plots["s2_plot"],
            self.sources["s2_source"],
            "s2",
            tooltips=s2_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_s2,
            lines=True,
            lower_bound=True,
            custom_colors="custom_colors",
        )
        s2_tab = Panel(child=self.plots["s2_plot"], title="Base 2")

        s_tabs = Tabs(name="s_tabs", tabs=[s10_tab, s2_tab], tabs_location="below")

        self.doc.add_root(s_tabs)

    def setup_widgets(self):
        # Initial selections

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
        self.widgets["select_test"] = Select(
            name="select_test", title="Test :", value=self.tests[0], options=self.tests
        )
        self.doc.add_root(self.widgets["select_test"])
        self.widgets["select_test"].on_change("value", self.update_test)
        self.widgets["select_test"].on_change("options", self.update_test)

        # Filter widget
        self.widgets["test_filter"] = TextInput(
            name="test_filter", title="Tests filter:"
        )
        self.widgets["test_filter"].js_on_change(
            "value",
            CustomJS(
                args=dict(options=self.tests, selector=self.widgets["select_test"]),
                code=filter_callback_js,
            ),
        )
        self.doc.add_root(self.widgets["test_filter"])

        # Number of runs to display

        self.widgets["select_n_runs"] = Select(
            name="select_n_runs",
            title="Display :",
            value=n_runs_display[1],
            options=n_runs_display,
        )
        self.doc.add_root(self.widgets["select_n_runs"])
        self.widgets["select_n_runs"].on_change("value", self.update_n_runs)

        # Variable selector widget

        self.widgets["select_var"] = Select(
            name="select_var", title="Variable :", value=self.vars[0], options=self.vars
        )
        self.doc.add_root(self.widgets["select_var"])
        self.widgets["select_var"].on_change("value", self.update_var)
        self.widgets["select_var"].on_change("options", self.update_var)

        # Backend selector widget

        self.widgets["select_backend"] = Select(
            name="select_backend",
            title="Verificarlo backend :",
            value=self.backends[0],
            options=self.backends,
        )
        self.doc.add_root(self.widgets["select_backend"])
        self.widgets["select_backend"].on_change("value", self.update_backend)

        # Outliers filtering checkbox

        self.widgets["outliers_filtering_compare"] = CheckboxGroup(
            name="outliers_filtering_compare", labels=["Filter outliers"], active=[]
        )
        self.doc.add_root(self.widgets["outliers_filtering_compare"])
        self.widgets["outliers_filtering_compare"].on_change(
            "active", self.update_outliers_filtering
        )

        # Communication methods
        # (to send/receive messages to/from master)

    def inspect_run_callback(self, new, source_name, x_name):
        """
        Callback to change view to "Inspect runs" when plot element is clicked
        """

        # In case we just unselected everything on the plot, then do nothing
        if not new:
            return

        # But if an element has been selected, go to the corresponding run
        index = new[-1]
        run_name = self.sources[source_name].data[x_name][index]

        self.master.go_to_inspect(run_name)

    # These are wrappers (one for each plot) for the above inspect_run_callback
    # function. Inside it, new is the index of the clicked element, and is
    # dependent of the plot because we could have filtered different outliers.
    # For this reason, we have one callback wrapper for each plot so we get
    # the correct element.

    def inspect_run_callback_boxplot(self, attr, old, new):
        self.inspect_run_callback(new, "boxplot_source", "x")

    def inspect_run_callback_sigma(self, attr, old, new):
        self.inspect_run_callback(new, "sigma_source", "sigma_x")

    def inspect_run_callback_s2(self, attr, old, new):
        self.inspect_run_callback(new, "s2_source", "s2_x")

    def inspect_run_callback_s10(self, attr, old, new):
        self.inspect_run_callback(new, "s10_source", "s10_x")

    def change_repo(self, new_data, new_metadata):
        """
        When received, update data and metadata with the new repo, and update
        everything
        """

        self.data = new_data
        self.metadata = new_metadata

        # Update widgets(and automatically trigger plot updates)
        self.tests = self.data.index.get_level_values("test").drop_duplicates().tolist()

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

        self.widgets["select_test"].options = self.tests
        self.widgets["select_test"].value = self.tests[0]

        # If changing repo doesn't affect the selection, trigger the callback
        # manually
        old = self.widgets["select_backend"].value
        if self.widgets["select_backend"].value == old:
            self.update_plots()

        # Constructor

    def __init__(self, master, doc, data, metadata):
        """
        Here are the most important attributes of the CompareRuns class

        master : reference to the ViewMaster class
        doc : an object provided by Bokeh to add elements to the HTML document
        data : pandas dataframe containing all the tests data
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

        self.sources = {
            "boxplot_source": ColumnDataSource(data={}),
            "sigma_source": ColumnDataSource(data={}),
            "s10_source": ColumnDataSource(data={}),
            "s2_source": ColumnDataSource(data={}),
        }

        self.plots = {}
        self.widgets = {}

        # Setup Bokeh objects
        self.setup_plots()
        self.setup_widgets()

        # At this point, everything should have been initialized, so we can
        # show the plots for the first time
        self.update_plots()
