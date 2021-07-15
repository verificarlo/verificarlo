#############################################################################
#                                                                           #
#  This file is part of Verificarlo.                                        #
#                                                                           #
#  Copyright (c) 2015-2021                                                  #
#     Verificarlo contributors                                              #
#     Universite de Versailles St-Quentin-en-Yvelines                       #
#     CMLA, Ecole Normale Superieure de Cachan                              #
#                                                                           #
#  Verificarlo is free software: you can redistribute it and/or modify      #
#  it under the terms of the GNU General Public License as published by     #
#  the Free Software Foundation, either version 3 of the License, or        #
#  (at your option) any later version.                                      #
#                                                                           #
#  Verificarlo is distributed in the hope that it will be useful,           #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#  GNU General Public License for more details.                             #
#                                                                           #
#  You should have received a copy of the GNU General Public License        #
#  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     #
#                                                                           #
#############################################################################

# General functions for filling plots with data in all report's views

from bokeh.plotting import figure
from bokeh.models import HoverTool, TapTool, CustomJS
from bokeh.colors import color
from bokeh.transform import dodge

from math import pi


def fill_dotplot(
    plot, source, data_field,
    tooltips=None, tooltips_formatters=None,
    js_tap_callback=None, server_tap_callback=None,
    lines=False,
    lower_bound=False,
    second_series=None,
    legend=None,
    second_legend=None,
    custom_colors=None
):
    '''
    General function for filling dotplots.
    Here are the possible parameters :

    tooltips: Bokeh Tooltip object to use for the plot
    tooltips_formatters: Formatter for the tooltip
    js_tap_callback: CustomJS object for client side click callback
    server_tap_callback: Callback object for server side click callback
    lines: Specify if lines should be drawn to connect the dots
    lower_bound: Specify if a lower bound interval should be displayed
    second_series: Name of a second data series to plot on the same figure. It
    should also have its own x series with the "_x" prefix.
    legend: lengend for the first data series
    second_legend: same for the second optional data series
    custom_colors: Will plot additional glyphs with a custom color (to display
    assert errors for instance). Should be the name of the series of colors.
    '''

    # (Optional) Tooltip and tooltip formatters
    if tooltips is not None:
        hover = HoverTool(tooltips=tooltips, mode="vline", names=["circle"])

        if tooltips_formatters is not None:
            hover.formatters = tooltips_formatters

        plot.add_tools(hover)

    # (Optional) Add TapTool (for JS tap callback)
    if js_tap_callback is not None:
        tap = TapTool(callback=CustomJS(code=js_tap_callback))
        plot.add_tools(tap)

    # (Optional) Add segment to represent a lower bound
    if lower_bound:
        lower_segment = plot.segment(
            x0="%s_x" % data_field, y0=data_field,
            x1="%s_x" % data_field, y1="%s_lower_bound" % data_field,
            source=source, line_color="black"
        )

    # (Optional) Draw a second data series
    if second_series is not None:
        # (Optional) Legend for the second data series
        if second_legend is not None:
            second_circle = plot.circle(
                name="second_circle",
                x="%s_x" % data_field, y=second_series, source=source, size=12,
                fill_color="grey", line_color="grey",
                legend_label=second_legend
            )
        else:
            second_circle = plot.circle(
                name="second_circle",
                x="%s_x" % data_field, y=second_series, source=source, size=12,
                fill_color="grey", line_color="grey"
            )

        if lines:
            second_line = plot.line(
                x="%s_x" % data_field, y=second_series, source=source,
                color="grey", line_dash="dashed"
            )

    # (Optional) Draw lines between dots
    if lines:
        line = plot.line(x="%s_x" % data_field, y=data_field, source=source)

    # Draw dots (actually Bokeh circles)
    # (Optional) Custom color palette (to display assert errors, for instance)
    if custom_colors is not None:
        # (Optional) Legend for the data series
        if legend is not None:
            circle = plot.circle(
                name="circle",
                x="%s_x" % data_field, y=data_field, source=source, size=12,
                legend_label=legend,
                fill_color=custom_colors, line_color=custom_colors
            )
        else:
            circle = plot.circle(
                name="circle",
                x="%s_x" % data_field, y=data_field, source=source, size=12,
                fill_color=custom_colors, line_color=custom_colors
            )

    else:
        # (Optional) Legend for the data series
        if legend is not None:
            circle = plot.circle(
                name="circle",
                x="%s_x" % data_field, y=data_field, source=source, size=12,
                legend_label=legend
            )
        else:
            circle = plot.circle(
                name="circle",
                x="%s_x" % data_field, y=data_field, source=source, size=12
            )

    # (Optional) Add server tap callback
    if server_tap_callback is not None:
        circle.data_source.selected.on_change("indices", server_tap_callback)

    # Plot appearance
    plot.xgrid.grid_line_color = None
    plot.ygrid.grid_line_color = None

    plot.yaxis[0].formatter.power_limit_high = 0
    plot.yaxis[0].formatter.power_limit_low = 0
    plot.yaxis[0].formatter.precision = 3

    plot.xaxis[0].major_label_orientation = pi / 8


def fill_boxplot(
    plot, source,
    prefix="",
    tooltips=None, tooltips_formatters=None,
    js_tap_callback=None, server_tap_callback=None,
    custom_colors=False
):
    '''
    General function for filling boxplots.
    Here are the possible parameters :

    prefix: specify which prefix to use for the name of data fields
    tooltips: Bokeh Tooltip object to use for the plot
    tooltips_formatters: Formatter for the tooltip
    js_tap_callback: CustomJS object for client side click callback
    server_tap_callback: Callback object for server side click callback
    custom_colors: Will plot additional glyphs with a custom color (to display
    assert errors for instance). Series of colors.
    '''

    # (Optional) Tooltip and tooltip formatters
    if tooltips is not None:
        hover = HoverTool(tooltips=tooltips, mode="vline", names=["full_box"])

        if tooltips_formatters is not None:
            hover.formatters = tooltips_formatters

        plot.add_tools(hover)

    # (Optional) Add TapTool (for JS tap callback)
    if js_tap_callback is not None:
        tap = TapTool(callback=CustomJS(code=js_tap_callback))
        plot.add_tools(tap)

    # Draw boxes (the prefix argument modifies the fields of ColumnDataSource
    # that are used)

    if prefix != "":
        prefix = "%s_" % prefix

    # Stems
    top_stem = plot.segment(
        x0="%sx" % prefix, y0="%smax" % prefix,
        x1="%sx" % prefix, y1="%squantile75" % prefix,
        source=source, line_color="black"
    )
    bottom_stem = plot.segment(
        x0="%sx" % prefix, y0="%smin" % prefix,
        x1="%sx" % prefix, y1="%squantile25" % prefix,
        source=source, line_color="black"
    )

    # Boxes

    if not custom_colors:
        full_box = plot.vbar(
            name="full_box",
            x="%sx" % prefix, width=0.5,
            top="%squantile75" % prefix, bottom="%squantile25" % prefix,
            source=source, line_color="black"
        )
        bottom_box = plot.vbar(
            x="%sx" % prefix, width=0.5,
            top="%squantile50" % prefix, bottom="%squantile25" % prefix,
            source=source, line_color="black"
        )

    # (Optional) Custom color palette (to display assert errors, for instance)
    else:
        full_box = plot.vbar(
            name="full_box",
            x="%sx" % prefix, width=0.5,
            top="%squantile75" % prefix, bottom="%squantile25" % prefix,
            source=source, line_color="black", fill_color="custom_colors"
        )
        bottom_box = plot.vbar(
            x="%sx" % prefix, width=0.5,
            top="%squantile50" % prefix, bottom="%squantile25" % prefix,
            source=source, line_color="black", fill_color="custom_colors"
        )

    # Mu dot
    mu_dot = plot.dot(
        x="%sx" % prefix, y="%smu" % prefix, size=30, source=source,
        color="black"
    )

    # (Optional) Add server tap callback
    if server_tap_callback is not None:
        top_stem.data_source.selected.on_change("indices", server_tap_callback)
        bottom_stem.data_source.selected.on_change(
            "indices", server_tap_callback)

        full_box.data_source.selected.on_change("indices", server_tap_callback)
        bottom_box.data_source.selected.on_change(
            "indices", server_tap_callback)

        mu_dot.data_source.selected.on_change("indices", server_tap_callback)

    # Plot appearance
    plot.xgrid.grid_line_color = None
    plot.ygrid.grid_line_color = None

    plot.yaxis[0].formatter.power_limit_high = 0
    plot.yaxis[0].formatter.power_limit_low = 0
    plot.yaxis[0].formatter.precision = 3

    plot.xaxis[0].major_label_orientation = pi / 8


def fill_barplot(
    plot, source,
    single_series=None, double_series=None,
    tooltips=None, tooltips_formatters=None,
    js_tap_callback=None, server_tap_callback=None,
):
    '''
    General function for filling barplots.
    Here are the possible parameters :

    single_series: Series that display one value at each x (string)
    double_series: Series that display two values at each x (list of strings, size 2)
    columns: Array of columns to display. Size should be coherent with "mode".
    legend: Array of texts to put in the legend. This should be specified when
    plotting more than one culum, and its size should be coherent with "mode".
    tooltips: Bokeh Tooltip object to use for the plot
    tooltips_formatters: Formatter for the tooltip
    js_tap_callback: CustomJS object for client side click callback
    server_tap_callback: Callback object for server side click callback
    '''

    vbars = []
    vbars_names = []

    # Draw "single" vbar
    if single_series is not None:
        vbar = plot.vbar(
            name="vbar",
            x="x", width=0.5,
            top=single_series,
            source=source
        )

        vbars.append(vbar)
        vbars_names.append("vbar")

    # Draw "double" vbars
    if double_series is not None:
        vbar1 = plot.vbar(
            name="vbar1",
            x=dodge("x", -0.15, range=plot.x_range), width=0.25,
            top=double_series[0],
            source=source
        )

        vbar2 = plot.vbar(
            name="vbar2",
            x=dodge("x", 0.15, range=plot.x_range), width=0.25,
            top=double_series[1],
            source=source,
            line_color="grey",
            fill_color="grey"
        )

        vbars.append(vbar1)
        vbars.append(vbar2)
        vbars_names.append("vbar1")
        vbars_names.append("vbar2")

    # (Optional) Tooltip and tooltip formatters
    if tooltips is not None:
        hover = HoverTool(tooltips=tooltips, mode="vline", names=vbars_names)

        if tooltips_formatters is not None:
            hover.formatters = tooltips_formatters

        plot.add_tools(hover)

    # (Optional) Add server tap callback
    if server_tap_callback is not None:
        for vbar in vbars:
            vbar.data_source.selected.on_change("indices", server_tap_callback)

    # Plot appearance
    plot.xgrid.grid_line_color = None
    plot.ygrid.grid_line_color = None

    plot.yaxis[0].formatter.power_limit_high = 0
    plot.yaxis[0].formatter.power_limit_low = 0
    plot.yaxis[0].formatter.precision = 3

    plot.xaxis[0].major_label_orientation = pi / 8
