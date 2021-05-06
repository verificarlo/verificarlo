# Manage the view comparing a variable over different runs

import time

import pandas as pd

from math import pi

from bokeh.plotting import figure, curdoc
from bokeh.embed import components
from bokeh.models import Select, ColumnDataSource, Panel, Tabs, HoverTool, \
    TextInput, CheckboxGroup, TapTool, CustomJS

import helper
import plot


##########################################################################


class CompareRuns:

    # Helper functions related to CompareRuns

    # From an array of timestamps, returns the array of runs names (for the x
    # axis ticks), as well as the metadata (in a dict of arrays) associated to
    # this array (for the tooltips)
    def gen_x_series(self, timestamps):

        # Initialize the objects to return
        x_series = []
        x_metadata = dict(
            date=[],
            is_git_commit=[],
            hash=[],
            author=[],
            message=[]
        )

        # n == 0 means we want all runs, we also make sure not to go out of
        # bound if asked for more runs than we have
        n = self.current_n_runs
        if n == 0 or n > len(timestamps):
            n = len(timestamps)

        for i in range(0, n):
            # Get metadata associated to this run
            row_metadata = helper.get_metadata(
                self.metadata, timestamps[-i - 1])
            date = time.ctime(timestamps[-i - 1])

            # Fill the x series
            str = row_metadata["name"]
            x_series.insert(0, helper.get_metadata(
                self.metadata, timestamps[-i - 1])["name"])

            # Fill the metadata lists
            x_metadata["date"].insert(0, date)
            x_metadata["is_git_commit"].insert(
                0, row_metadata["is_git_commit"])
            x_metadata["hash"].insert(0, row_metadata["hash"])
            x_metadata["author"].insert(0, row_metadata["author"])
            x_metadata["message"].insert(0, row_metadata["message"])

        return x_series, x_metadata

        # Plots update function

    def update_plots(self):

        # Select all data matching current test/var/backend

        runs = self.data.loc[[self.widgets["select_test"].value],
                             self.widgets["select_var"].value,
                             self.widgets["select_backend"].value]

        timestamps = runs["timestamp"]
        x_series, x_metadata = self.gen_x_series(timestamps.sort_values())

        # Update source

        main_dict = runs.to_dict("series")
        main_dict["x"] = x_series

        # Add metadata (for tooltip)
        main_dict.update(x_metadata)

        # Select the last n runs only
        n = self.current_n_runs
        main_dict = {key: value[-n:] for key, value in main_dict.items()}

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
            }

            if stat == "s10" or stat == "s2":
                dict["%s_lower_bound" %
                     stat] = main_dict["%s_lower_bound" %
                                       stat]

            # Filter outliers if the box is checked
            if len(self.widgets["outliers_filtering_compare"].active) > 0:
                outliers = helper.detect_outliers(dict[stat])
                dict[stat] = helper.remove_outliers(dict[stat], outliers)
                dict["%s_x" % stat] = helper.remove_outliers(
                    dict["%s_x" % stat], outliers)

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

            "nsamples": main_dict["nsamples"]
        }

        self.sources["boxplot_source"].data = dict

        # Update x axis

        helper.reset_x_range(
            self.plots["boxplot"],
            self.sources["boxplot_source"].data["x"]
        )
        helper.reset_x_range(
            self.plots["sigma_plot"],
            self.sources["sigma_source"].data["sigma_x"]
        )
        helper.reset_x_range(
            self.plots["s10_plot"],
            self.sources["s10_source"].data["s10_x"]
        )
        helper.reset_x_range(
            self.plots["s2_plot"],
            self.sources["s2_source"].data["s2_x"]
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
        self.vars = self.data.loc[new]\
            .index.get_level_values("variable").drop_duplicates().tolist()
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
        self.backends = self.data.loc[self.widgets["select_test"].value, self.widgets["select_var"].value]\
            .index.get_level_values("vfc_backend").drop_duplicates().tolist()
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
        self.select_n_runs.value = new
        self.current_n_runs = self.n_runs_dict[self.select_n_runs.value]

        self.update_plots()

    def update_outliers_filtering(self, attrname, old, new):
        self.update_plots()

        # Bokeh setup functions

    def setup_plots(self):

        tools = "pan, wheel_zoom, xwheel_zoom, ywheel_zoom, reset, save"

        # Custom JS callback that will be used when tapping on a run
        # Only switches the view, a server callback is required to update plots
        # (defined inside template to avoid bloating server w/ too much JS code)
        js_tap_callback = "goToInspectRuns();"

        # Box plot
        self.plots["boxplot"] = figure(
            name="boxplot", title="Variable distribution over runs",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
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
            ("Number of samples", "@nsamples")
        ]
        box_tooltips_formatters = {
            "@min": "printf",
            "@max": "printf",
            "@quantile25": "printf",
            "@quantile50": "printf",
            "@quantile75": "printf",
            "@mu": "printf"
        }

        plot.fill_boxplot(
            self.plots["boxplot"], self.sources["boxplot_source"],
            tooltips=box_tooltips,
            tooltips_formatters=box_tooltips_formatters,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_boxplot,
        )
        self.doc.add_root(self.plots["boxplot"])

        # Sigma plot (bar plot)
        self.plots["sigma_plot"] = figure(
            name="sigma_plot", title="Standard deviation σ over runs",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
        )

        sigma_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("σ", "@sigma"),
            ("Number of samples", "@nsamples")
        ]

        plot.fill_dotplot(
            self.plots["sigma_plot"], self.sources["sigma_source"], "sigma",
            tooltips=sigma_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_sigma,
            lines=True
        )
        self.doc.add_root(self.plots["sigma_plot"])

        # s plot (bar plot with 2 tabs)
        self.plots["s10_plot"] = figure(
            name="s10_plot", title="Significant digits s over runs",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
        )

        s10_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("s", "@s10"),
            ("s lower bound", "@s10_lower_bound"),
            ("Number of samples", "@nsamples")
        ]

        plot.fill_dotplot(
            self.plots["s10_plot"], self.sources["s10_source"], "s10",
            tooltips=s10_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_s10,
            lines=True,
            lower_bound=True
        )
        s10_tab = Panel(child=self.plots["s10_plot"], title="Base 10")

        self.plots["s2_plot"] = figure(
            name="s2_plot", title="Significant digits s over runs",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
        )

        s2_tooltips = [
            ("Git commit", "@is_git_commit"),
            ("Date", "@date"),
            ("Hash", "@hash"),
            ("Author", "@author"),
            ("Message", "@message"),
            ("s", "@s2"),
            ("s lower bound", "@s2_lower_bound"),
            ("Number of samples", "@nsamples")
        ]

        plot.fill_dotplot(
            self.plots["s2_plot"], self.sources["s2_source"], "s2",
            tooltips=s2_tooltips,
            js_tap_callback=js_tap_callback,
            server_tap_callback=self.inspect_run_callback_s2,
            lines=True,
            lower_bound=True
        )
        s2_tab = Panel(child=self.plots["s2_plot"], title="Base 2")

        s_tabs = Tabs(
            name="s_tabs",
            tabs=[s10_tab, s2_tab],
            tabs_location="below"
        )

        self.doc.add_root(s_tabs)

    def setup_widgets(self):

        # Initial selections

        # Test/var/backend combination (we select all first elements at init)
        self.tests = self.data\
            .index.get_level_values("test").drop_duplicates().tolist()

        self.vars = self.data.loc[self.tests[0]]\
            .index.get_level_values("variable").drop_duplicates().tolist()

        self.backends = self.data.loc[self.tests[0], self.vars[0]]\
            .index.get_level_values("vfc_backend").drop_duplicates().tolist()

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
            "All runs": 0
        }

        # Contains all options strings
        n_runs_display = list(self.n_runs_dict.keys())

        # Will be used when updating plots (contains actual number to diplay)
        self.current_n_runs = self.n_runs_dict[n_runs_display[1]]

        # Selector widget
        self.widgets["select_test"] = Select(
            name="select_test", title="Test :",
            value=self.tests[0], options=self.tests
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
                args=dict(
                    options=self.tests,
                    selector=self.widgets["select_test"]),
                code=filter_callback_js))
        self.doc.add_root(self.widgets["test_filter"])

        # Number of runs to display

        self.widgets["select_n_runs"] = Select(
            name="select_n_runs", title="Display :",
            value=n_runs_display[1], options=n_runs_display
        )
        self.doc.add_root(self.widgets["select_n_runs"])
        self.widgets["select_n_runs"].on_change("value", self.update_n_runs)

        # Variable selector widget

        self.widgets["select_var"] = Select(
            name="select_var", title="Variable :",
            value=self.vars[0], options=self.vars
        )
        self.doc.add_root(self.widgets["select_var"])
        self.widgets["select_var"].on_change("value", self.update_var)
        self.widgets["select_var"].on_change("options", self.update_var)

        # Backend selector widget

        self.widgets["select_backend"] = Select(
            name="select_backend", title="Verificarlo backend :",
            value=self.backends[0], options=self.backends
        )
        self.doc.add_root(self.widgets["select_backend"])
        self.widgets["select_backend"].on_change("value", self.update_backend)

        # Outliers filtering checkbox

        self.widgets["outliers_filtering_compare"] = CheckboxGroup(
            name="outliers_filtering_compare",
            labels=["Filter outliers"], active=[]
        )
        self.doc.add_root(self.widgets["outliers_filtering_compare"])
        self.widgets["outliers_filtering_compare"]\
            .on_change("active", self.update_outliers_filtering)

        # Communication methods
        # (to send/receive messages to/from master)

    # Callback to change view of Inspect runs when data is selected

    def inspect_run_callback(self, new, source_name, x_name):

        # In case we just unselected everything, then do nothing
        if new == []:
            return

        index = new[-1]
        run_name = self.sources[source_name].data[x_name][index]

        self.master.go_to_inspect(run_name)

    # Wrappers for each plot (since new is the index of the clicked element,
    # it is dependent of the plot because we could have filtered some outliers)
    # There doesn't seem to be an easy way to add custom parameters to a
    # Bokeh callback, so using wrappers seems to be the best solution for now

    def inspect_run_callback_boxplot(self, attr, old, new):
        self.inspect_run_callback(new, "boxplot_source", "x")

    def inspect_run_callback_sigma(self, attr, old, new):
        self.inspect_run_callback(new, "sigma_source", "sigma_x")

    def inspect_run_callback_s2(self, attr, old, new):
        self.inspect_run_callback(new, "s2_source", "s2_x")

    def inspect_run_callback_s10(self, attr, old, new):
        self.inspect_run_callback(new, "s10_source", "s10_x")

        # Constructor

    def __init__(self, master, doc, data, metadata):

        self.master = master

        self.doc = doc
        self.data = data
        self.metadata = metadata

        self.sources = {
            "boxplot_source": ColumnDataSource(data={}),
            "sigma_source": ColumnDataSource(data={}),
            "s10_source": ColumnDataSource(data={}),
            "s2_source": ColumnDataSource(data={})
        }

        self.plots = {}
        self.widgets = {}

        # Setup Bokeh objects
        self.setup_plots()
        self.setup_widgets()

        # At this point, everything should have been initialized, so we can
        # show the plots for the first time
        self.update_plots()
