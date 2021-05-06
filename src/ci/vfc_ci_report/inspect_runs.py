# Manage the view comparing the variables of a run

from math import pi
from functools import partial

import pandas as pd
import numpy as np

from bokeh.plotting import figure, curdoc
from bokeh.embed import components
from bokeh.models import Select, ColumnDataSource, Panel, Tabs, HoverTool,\
    RadioButtonGroup, CheckboxGroup, CustomJS

import helper
import plot


##########################################################################


class InspectRuns:

    # Helper functions related to InspectRun

    # Returns a dictionary mapping user-readable strings to all run timestamps
    def gen_runs_selection(self):

        runs_dict = {}

        # Iterate over timestamp rows (runs) and fill dict
        for row in self.metadata.iloc:
            # The syntax used by pandas makes this part a bit tricky :
            # row.name is the index of metadata (so it refers to the
            # timestamp), whereas row["name"] is the column called "name"
            # (which is the display string used for the run)

            # runs_dict[run's name] = run's timestamp
            runs_dict[row["name"]] = row.name

        return runs_dict

    def gen_boxplot_tooltips(self, prefix):
        return [
            ("Name", "@%s_x" % prefix),
            ("Min", "@" + prefix + "_min{%0.18e}"),
            ("Max", "@" + prefix + "_max{%0.18e}"),
            ("1st quartile", "@" + prefix + "_quantile25{%0.18e}"),
            ("Median", "@" + prefix + "_quantile50{%0.18e}"),
            ("3rd quartile", "@" + prefix + "_quantile75{%0.18e}"),
            ("μ", "@" + prefix + "_mu{%0.18e}"),
            ("Number of samples (tests)", "@nsamples")
        ]

    def gen_boxplot_tooltips_formatters(self, prefix):
        return {
            "@%s_min" % prefix: "printf",
            "@%s_max" % prefix: "printf",
            "@%s_quantile25" % prefix: "printf",
            "@%s_quantile50" % prefix: "printf",
            "@%s_quantile75" % prefix: "printf",
            "@%s_mu" % prefix: "printf"
        }

    # Data processing helper
    # (computes new distributions for sigma, s2, s10)

    def data_processing(self, dataframe):

        # Compute aggragated mu
        dataframe["mu"] = np.vectorize(
            np.average)(
            dataframe["mu"],
            weights=dataframe["nsamples"])

        # nsamples is the number of aggregated elements (as well as the number
        # of samples for our new sigma and s distributions)
        dataframe["nsamples"] = dataframe["nsamples"].apply(lambda x: len(x))

        dataframe["mu_x"] = dataframe.index
        # Make sure that strings don't excede a certain length
        dataframe["mu_x"] = dataframe["mu_x"].apply(
            lambda x: x[:17] + "[...]" + x[-17:] if len(x) > 39 else x
        )

        # Get quantiles and mu for sigma, s10, s2
        for prefix in ["sigma", "s10", "s2"]:

            dataframe["%s_x" % prefix] = dataframe["mu_x"]

            dataframe[prefix] = dataframe[prefix].apply(np.sort)

            dataframe["%s_min" % prefix] = dataframe[prefix].apply(np.min)
            dataframe["%s_quantile25" % prefix] = dataframe[prefix].apply(
                np.quantile, args=(0.25,))
            dataframe["%s_quantile50" % prefix] = dataframe[prefix].apply(
                np.quantile, args=(0.50,))
            dataframe["%s_quantile75" % prefix] = dataframe[prefix].apply(
                np.quantile, args=(0.75,))
            dataframe["%s_max" % prefix] = dataframe[prefix].apply(np.max)
            dataframe["%s_mu" % prefix] = dataframe[prefix].apply(np.average)
            del dataframe[prefix]

        return dataframe

        # Plots update function

    def update_plots(self):

        groupby_display = self.widgets["groupby_radio"].labels[
            self.widgets["groupby_radio"].active
        ]
        groupby = self.factors_dict[groupby_display]

        filterby_display = self.widgets["filterby_radio"].labels[
            self.widgets["filterby_radio"].active
        ]
        filterby = self.factors_dict[filterby_display]

        # Groupby and aggregate lines belonging to the same group in lists

        groups = self.run_data[
            self.run_data.index.isin(
                [self.widgets["select_filter"].value],
                level=filterby
            )
        ].groupby(groupby)

        groups = groups.agg({
            "sigma": lambda x: x.tolist(),
            "s10": lambda x: x.tolist(),
            "s2": lambda x: x.tolist(),

            "mu": lambda x: x.tolist(),

            # Used for mu weighted average first, then will be replaced
            "nsamples": lambda x: x.tolist()
        })

        # Compute the new distributions, ...
        groups = self.data_processing(groups).to_dict("list")

        # Update source

        # Assign each ColumnDataSource, starting with the boxplots
        for prefix in ["sigma", "s10", "s2"]:

            dict = {
                "%s_x" % prefix: groups["%s_x" % prefix],
                "%s_min" % prefix: groups["%s_min" % prefix],
                "%s_quantile25" % prefix: groups["%s_quantile25" % prefix],
                "%s_quantile50" % prefix: groups["%s_quantile50" % prefix],
                "%s_quantile75" % prefix: groups["%s_quantile75" % prefix],
                "%s_max" % prefix: groups["%s_max" % prefix],
                "%s_mu" % prefix: groups["%s_mu" % prefix],

                "nsamples": groups["nsamples"]
            }

            # Filter outliers if the box is checked
            if len(self.widgets["outliers_filtering_inspect"].active) > 0:

                # Boxplots will be filtered by max then min
                top_outliers = helper.detect_outliers(dict["%s_max" % prefix])
                helper.remove_boxplot_outliers(dict, top_outliers, prefix)

                bottom_outliers = helper.detect_outliers(
                    dict["%s_min" % prefix])
                helper.remove_boxplot_outliers(dict, bottom_outliers, prefix)

            self.sources["%s_source" % prefix].data = dict

        # Finish with the mu plot
        dict = {
            "mu_x": groups["mu_x"],
            "mu": groups["mu"],

            "nsamples": groups["nsamples"]
        }

        self.sources["mu_source"].data = dict

        # Filter outliers if the box is checked
        if len(self.widgets["outliers_filtering_inspect"].active) > 0:
            mu_outliers = helper.detect_outliers(groups["mu"])
            groups["mu"] = helper.remove_outliers(groups["mu"], mu_outliers)
            groups["mu_x"] = helper.remove_outliers(
                groups["mu_x"], mu_outliers)

            # Update plots axis/titles

        # Get display string of the last (unselected) factor
        factors_dict = self.factors_dict.copy()
        del factors_dict[groupby_display]
        del factors_dict[filterby_display]
        for_all = list(factors_dict.keys())[0]

        # Update all display strings for plot title (remove caps, plural)
        groupby_display = groupby_display.lower()
        filterby_display = filterby_display.lower()[:-1]
        for_all = for_all.lower()

        self.plots["mu_inspect"].title.text = \
            "Empirical average μ of %s (groupped by %s, for all %s)" \
            % (filterby_display, groupby_display, for_all)

        self.plots["sigma_inspect"].title.text = \
            "Standard deviation σ of %s (groupped by %s, for all %s)" \
            % (filterby_display, groupby_display, for_all)

        self.plots["s10_inspect"].title.text = \
            "Significant digits s of %s (groupped by %s, for all %s)" \
            % (filterby_display, groupby_display, for_all)

        self.plots["s2_inspect"].title.text = \
            "Significant digits s of %s (groupped by %s, for all %s)" \
            % (filterby_display, groupby_display, for_all)

        helper.reset_x_range(self.plots["mu_inspect"], groups["mu_x"])
        helper.reset_x_range(self.plots["sigma_inspect"], groups["sigma_x"])
        helper.reset_x_range(self.plots["s10_inspect"], groups["s10_x"])
        helper.reset_x_range(self.plots["s2_inspect"], groups["s2_x"])

        # Widets' callback functions

    # Run selector callback

    def update_run(self, attrname, old, new):

        filterby = self.widgets["filterby_radio"].labels[
            self.widgets["filterby_radio"].active
        ]
        filterby = self.factors_dict[filterby]

        # Update run selection (by using dict mapping)
        self.current_run = self.runs_dict[new]

        # Update run data
        self.run_data = self.data[self.data["timestamp"] == self.current_run]

        # Save old selected option
        old_value = self.widgets["select_filter"].value

        # Update filter options
        options = self.run_data.index\
            .get_level_values(filterby).drop_duplicates().tolist()
        self.widgets["select_filter"].options = options

        if old_value not in self.widgets["select_filter"].options:
            self.widgets["select_filter"].value = options[0]
            # The update_var callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_filter("", "", old_value)

    # "Group by" radio

    def update_groupby(self, attrname, old, new):

        # Update "Filter by" radio list
        filterby_list = list(self.factors_dict.keys())
        del filterby_list[self.widgets["groupby_radio"].active]
        self.widgets["filterby_radio"].labels = filterby_list

        filterby = self.widgets["filterby_radio"].labels[
            self.widgets["filterby_radio"].active
        ]
        filterby = self.factors_dict[filterby]

        # Save old selected option
        old_value = self.widgets["select_filter"].value

        # Update filter options
        options = self.run_data.index\
            .get_level_values(filterby).drop_duplicates().tolist()
        self.widgets["select_filter"].options = options

        if old_value not in self.widgets["select_filter"].options:
            self.widgets["select_filter"].value = options[0]
            # The update_var callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_filter("", "", old_value)

    # "Filter by" radio

    def update_filterby(self, attrname, old, new):

        filterby = self.widgets["filterby_radio"].labels[
            self.widgets["filterby_radio"].active
        ]
        filterby = self.factors_dict[filterby]

        # Save old selected option
        old_value = self.widgets["select_filter"].value

        # Update filter selector options
        options = self.run_data.index\
            .get_level_values(filterby).drop_duplicates().tolist()
        self.widgets["select_filter"].options = options

        if old_value not in self.widgets["select_filter"].options:
            self.widgets["select_filter"].value = options[0]
            # The update_var callback will be triggered by the assignment

        else:
            # Trigger the callback manually (since the plots need to be updated
            # anyway)
            self.update_filter("", "", old_value)

    # Filter selector callback

    def update_filter(self, attrname, old, new):
        self.update_plots()

    # Filter outliers checkbox callback

    def update_outliers_filtering(self, attrname, old, new):
        # The status (checked/unchecked) of the checkbox is also verified inside
        # self.update_plots(), so calling this function is enough
        self.update_plots()

        # Bokeh setup functions
        # (for both variable and backend selection at once)

    def setup_plots(self):

        tools = "pan, wheel_zoom, xwheel_zoom, ywheel_zoom, reset, save"

        # Tooltips and formatters

        dotplot_tooltips = [
            ("Name", "@mu_x"),
            ("μ", "@mu{%0.18e}"),
            ("Number of samples (tests)", "@nsamples")
        ]
        dotplot_formatters = {
            "@mu": "printf"
        }

        sigma_boxplot_tooltips = self.gen_boxplot_tooltips("sigma")
        sigma_boxplot_tooltips_formatters = self.gen_boxplot_tooltips_formatters(
            "sigma")

        s10_boxplot_tooltips = self.gen_boxplot_tooltips("s10")
        s10_boxplot_tooltips_formatters = self.gen_boxplot_tooltips_formatters(
            "s10")

        s2_boxplot_tooltips = self.gen_boxplot_tooltips("s2")
        s2_boxplot_tooltips_formatters = self.gen_boxplot_tooltips_formatters(
            "s2")

        # Plots

        # Mu plot
        self.plots["mu_inspect"] = figure(
            name="mu_inspect",
            title="",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
        )
        plot.fill_dotplot(
            self.plots["mu_inspect"], self.sources["mu_source"], "mu",
            tooltips=dotplot_tooltips,
            tooltips_formatters=dotplot_formatters
        )
        self.doc.add_root(self.plots["mu_inspect"])

        # Sigma plot
        self.plots["sigma_inspect"] = figure(
            name="sigma_inspect",
            title="",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode="scale_width"
        )
        plot.fill_boxplot(
            self.plots["sigma_inspect"],
            self.sources["sigma_source"],
            prefix="sigma",
            tooltips=sigma_boxplot_tooltips,
            tooltips_formatters=sigma_boxplot_tooltips_formatters)
        self.doc.add_root(self.plots["sigma_inspect"])

        # s plots
        self.plots["s10_inspect"] = figure(
            name="s10_inspect",
            title="",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode='scale_width'
        )
        plot.fill_boxplot(
            self.plots["s10_inspect"],
            self.sources["s10_source"],
            prefix="s10",
            tooltips=s10_boxplot_tooltips,
            tooltips_formatters=s10_boxplot_tooltips_formatters)
        s10_tab_inspect = Panel(
            child=self.plots["s10_inspect"],
            title="Base 10")

        self.plots["s2_inspect"] = figure(
            name="s2_inspect",
            title="",
            plot_width=900, plot_height=400, x_range=[""],
            tools=tools, sizing_mode='scale_width'
        )
        plot.fill_boxplot(
            self.plots["s2_inspect"], self.sources["s2_source"], prefix="s2",
            tooltips=s2_boxplot_tooltips,
            tooltips_formatters=s2_boxplot_tooltips_formatters
        )
        s2_tab_inspect = Panel(child=self.plots["s2_inspect"], title="Base 2")

        s_tabs_inspect = Tabs(
            name="s_tabs_inspect",
            tabs=[s10_tab_inspect, s2_tab_inspect], tabs_location="below"
        )
        self.doc.add_root(s_tabs_inspect)

    def setup_widgets(self):

        # Generation of selectable items

        # Dict contains all inspectable runs (maps display strings to timestamps)
        # The dict structure allows to get the timestamp from the display string
        # in O(1)
        self.runs_dict = self.gen_runs_selection()

        # Dict maps display strings to column names for the different factors
        # (var, backend, test)
        self.factors_dict = {
            "Variables": "variable",
            "Backends": "vfc_backend",
            "Tests": "test"
        }

        # Run selection

        # Contains all options strings
        runs_display = list(self.runs_dict.keys())
        # Will be used when updating plots (contains actual number)
        self.current_run = self.runs_dict[runs_display[-1]]
        # Contains the selected option string, used to update current_n_runs
        current_run_display = runs_display[-1]
        # This contains only entries matching the run
        self.run_data = self.data[self.data["timestamp"] == self.current_run]

        change_run_callback_js = "updateRunMetadata(cb_obj.value);"

        self.widgets["select_run"] = Select(
            name="select_run", title="Run :",
            value=current_run_display, options=runs_display
        )
        self.doc.add_root(self.widgets["select_run"])
        self.widgets["select_run"].on_change("value", self.update_run)
        self.widgets["select_run"].js_on_change("value", CustomJS(
            code=change_run_callback_js,
            args=(dict(
                metadata=helper.metadata_to_dict(
                    helper.get_metadata(self.metadata, self.current_run)
                )
            ))
        ))

        # Factors selection

        # "Group by" radio
        self.widgets["groupby_radio"] = RadioButtonGroup(
            name="groupby_radio",
            labels=list(self.factors_dict.keys()), active=0
        )
        self.doc.add_root(self.widgets["groupby_radio"])
        # The functions are defined inside the template to avoid writing too
        # much JS server side
        self.widgets["groupby_radio"].on_change(
            "active",
            self.update_groupby
        )

        # "Filter by" radio
        # Get all possible factors, and remove the one selected in "Group by"
        filterby_list = list(self.factors_dict.keys())
        del filterby_list[self.widgets["groupby_radio"].active]

        self.widgets["filterby_radio"] = RadioButtonGroup(
            name="filterby_radio",
            labels=filterby_list, active=0
        )
        self.doc.add_root(self.widgets["filterby_radio"])
        # The functions are defined inside the template to avoid writing too
        # much JS server side
        self.widgets["filterby_radio"].on_change(
            "active",
            self.update_filterby
        )

        # Filter selector

        filterby = self.widgets["filterby_radio"].labels[
            self.widgets["filterby_radio"].active
        ]
        filterby = self.factors_dict[filterby]

        options = self.run_data.index\
            .get_level_values(filterby).drop_duplicates().tolist()

        self.widgets["select_filter"] = Select(
            # We need a different name to avoid collision in the template with
            # the runs comparison's widget
            name="select_filter", title="Select a filter :",
            value=options[0], options=options
        )
        self.doc.add_root(self.widgets["select_filter"])
        self.widgets["select_filter"]\
            .on_change("value", self.update_filter)

        # Toggle for outliers filtering

        self.widgets["outliers_filtering_inspect"] = CheckboxGroup(
            name="outliers_filtering_inspect",
            labels=["Filter outliers"], active=[]
        )
        self.doc.add_root(self.widgets["outliers_filtering_inspect"])
        self.widgets["outliers_filtering_inspect"]\
            .on_change("active", self.update_outliers_filtering)

        # Communication methods
        # (to send/receive messages to/from master)

    # When received, switch to the run_name in parameter

    def switch_view(self, run_name):
        self.widgets["select_run"].value = run_name

        # Constructor

    def __init__(self, master, doc, data, metadata):

        self.master = master

        self.doc = doc
        self.data = data
        self.metadata = metadata

        self.sources = {
            "mu_source": ColumnDataSource(data={}),
            "sigma_source": ColumnDataSource(data={}),
            "s10_source": ColumnDataSource(data={}),
            "s2_source": ColumnDataSource(data={})
        }

        self.plots = {}
        self.widgets = {}

        # Setup Bokeh objects
        self.setup_plots()
        self.setup_widgets()

        # Pass the initial metadata to the template (will be updated in CustomJS
        # callbacks). This is required because metadata is not displayed in a
        # Bokeh widget, so we can't update this with a server callback.
        initial_run = helper.get_metadata(self.metadata, self.current_run)
        self.doc.template_variables["initial_timestamp"] = self.current_run

        # At this point, everything should have been initialized, so we can
        # show the plots for the first time
        self.update_plots()
