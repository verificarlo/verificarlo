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

# Manage the view comparing the checks by run.
# At its creation, a Checks object will create all the needed Bokeh widgets,
# setup the callback functions (either server side or client side),
# initialize widgets selection, and from this selection generate the first plots.
# Then, when callback functions are triggered, widgets selections are updated,
# and re-generated with the newly selected data.
# This manages both deterministic and non-deterministic views in the report !

import helper
from bokeh.models import ColumnDataSource, CustomJS, DataTable, Select, TableColumn

##########################################################################


class Checks:
    # Widgets' callback functions

    def update_non_deterministic_run(self, attrname, old, new):
        # Update run selection (by using dict mapping)
        self.current_non_deterministic_run = self.runs_dict[new]

        # Update run data
        self.run_data = self.data[
            self.data["timestamp"] == self.current_non_deterministic_run
        ]

        # Only keep interesting columns
        self.run_data = self.run_data[
            [
                "check",
                "accuracy_threshold",
                "check_mode",
                "mu",
                "sigma",
            ]
        ]

        # Only keep probes that have an check
        self.run_data = self.run_data[self.run_data.accuracy_threshold != 0]

        # Uppercase first letter of "check_mode" for display
        self.run_data["check_mode"] = self.run_data["check_mode"].apply(
            lambda x: x.capitalize()
        )

        # Generate the data source object
        self.run_data.reset_index(inplace=True)
        self.sources["non_deterministic"].data = self.run_data

    def update_deterministic_run(self, attrname, old, new):
        # Update run selection (by using dict mapping)
        self.current_deterministic_run = self.runs_dict[new]

        # Update run data
        self.deterministic_run_data = self.deterministic_data[
            self.deterministic_data["timestamp"] == self.current_run
        ]

        # Only keep interesting columns
        self.deterministic_run_data = self.deterministic_run_data[
            [
                "check",
                "accuracy_threshold",
                "check_mode",
                "value",
                "reference_value",
            ]
        ]

        # Only keep probes that have an check
        self.deterministic_run_data = self.deterministic_run_data[
            self.deterministic_run_data.accuracy_threshold != 0
        ]

        # Uppercase first letter of "check_mode" for display
        self.deterministic_run_data["check_mode"] = self.deterministic_run_data[
            "check_mode"
        ].apply(lambda x: x.capitalize())

        # Generate the data source object
        self.deterministic_run_data.reset_index(inplace=True)
        self.sources["deterministic"].data = self.deterministic_run_data

        # Bokeh setup functions

    def setup_widgets(self):
        # Run selection

        # Dict contains all inspectable runs (maps display strings to timestamps)
        # The dict structure allows to get the timestamp from the display string
        # in O(1)
        self.runs_dict = helper.gen_runs_selection(self.metadata)

        # Contains all options strings
        runs_display = list(self.runs_dict.keys())
        # Will be used when updating plots (contains actual number)
        self.current_run = self.runs_dict[runs_display[-1]]
        self.current_deterministic_run = self.runs_dict[runs_display[-1]]

        # Contains the selected option string, used to update current_n_runs
        current_run_display = runs_display[-1]

        # This contains only entries matching the run
        self.run_data = self.data[self.data["timestamp"] == self.current_run]
        self.deterministic_run_data = self.deterministic_data[
            self.deterministic_data["timestamp"] == self.current_deterministic_run
        ]

        # Only keep interesting columns
        self.run_data = self.run_data[
            [
                "check",
                "accuracy_threshold",
                "check_mode",
                "mu",
                "sigma",
            ]
        ]

        self.deterministic_run_data = self.deterministic_run_data[
            [
                "check",
                "accuracy_threshold",
                "check_mode",
                "value",
                "reference_value",
            ]
        ]

        # Only keep probes that have an check
        self.run_data = self.run_data[self.run_data.accuracy_threshold != 0]

        self.deterministic_run_data = self.deterministic_run_data[
            self.deterministic_run_data.accuracy_threshold != 0
        ]

        # Uppercase first letter of "check_mode" for display
        self.run_data["check_mode"] = self.run_data["check_mode"].apply(
            lambda x: x.capitalize()
        )
        self.deterministic_run_data["check_mode"] = self.deterministic_run_data[
            "check_mode"
        ].apply(lambda x: x.capitalize())

        # Generate the data source objects
        self.run_data.reset_index(inplace=True)
        self.deterministic_run_data.reset_index(inplace=True)
        self.sources["non_deterministic"].data = self.run_data
        self.sources["deterministic"].data = self.deterministic_run_data

        # Non-deterministic run selector
        change_run_callback_js = (
            'updateRunMetadata(cb_obj.value, "non-deterministic-checks-");'
        )

        self.widgets["select_check_run_non_deterministic"] = Select(
            name="select_check_run_non_deterministic",
            title="Run :",
            value=current_run_display,
            options=runs_display,
        )
        self.doc.add_root(self.widgets["select_check_run_non_deterministic"])
        self.widgets["select_check_run_non_deterministic"].on_change(
            "value", self.update_non_deterministic_run
        )
        self.widgets["select_check_run_non_deterministic"].js_on_change(
            "value",
            CustomJS(
                code=change_run_callback_js,
                args=(
                    dict(
                        metadata=helper.metadata_to_dict(
                            helper.get_metadata(self.metadata, self.current_run)
                        )
                    )
                ),
            ),
        )

        # Deterministic run selector
        change_run_callback_js = (
            'updateRunMetadata(cb_obj.value, "deterministic-checks-");'
        )

        self.widgets["select_check_run_deterministic"] = Select(
            name="select_check_run_deterministic",
            title="Run :",
            value=current_run_display,
            options=runs_display,
        )
        self.doc.add_root(self.widgets["select_check_run_deterministic"])
        self.widgets["select_check_run_deterministic"].on_change(
            "value", self.update_deterministic_run
        )
        self.widgets["select_check_run_deterministic"].js_on_change(
            "value",
            CustomJS(
                code=change_run_callback_js,
                args=(
                    dict(
                        metadata=helper.metadata_to_dict(
                            helper.get_metadata(
                                self.metadata, self.current_deterministic_run
                            )
                        )
                    )
                ),
            ),
        )

        # Non-deterministic checks table:

        non_deterministic_olumns = [
            TableColumn(field="test", title="Test"),
            TableColumn(field="variable", title="Variable"),
            TableColumn(field="vfc_backend", title="Backend"),
            TableColumn(field="accuracy_threshold", title="Target precision"),
            TableColumn(field="check_mode", title="Check mode"),
            TableColumn(field="mu", title="Emp. avg. μ"),
            TableColumn(field="sigma", title="Std. dev. σ"),
            TableColumn(field="check", title="Passed"),
        ]

        self.widgets["non_deterministic_checks_table"] = DataTable(
            name="non_deterministic_checks_table",
            source=self.sources["non_deterministic"],
            columns=non_deterministic_olumns,
            width=895,
            sizing_mode="scale_width",
        )
        self.doc.add_root(self.widgets["non_deterministic_checks_table"])

        # Deterministic checks table:

        deterministic_columns = [
            TableColumn(field="test", title="Test"),
            TableColumn(field="variable", title="Variable"),
            TableColumn(field="vfc_backend", title="Backend"),
            TableColumn(field="accuracy_threshold", title="Target precision"),
            TableColumn(field="check_mode", title="Check mode"),
            TableColumn(field="value", title="Backend value"),
            TableColumn(field="reference_value", title="IEEE value"),
            TableColumn(field="check", title="Passed"),
        ]

        self.widgets["deterministic_checks_table"] = DataTable(
            name="deterministic_checks_table",
            source=self.sources["deterministic"],
            columns=deterministic_columns,
            width=895,
            sizing_mode="scale_width",
        )
        self.doc.add_root(self.widgets["deterministic_checks_table"])

        # Communication methods
        # (to send/receive messages to/from master)

    def change_repo(self, new_data, new_deterministic_data, new_metadata):
        """
        When received, update data and metadata with the new repo, and update
        everything
        """

        self.data = new_data
        self.deterministic_data = new_deterministic_data
        self.metadata = new_metadata

        self.runs_dict = helper.gen_runs_selection(self.metadata)

        # Contains all options strings
        runs_display = list(self.runs_dict.keys())
        # Will be used when updating plots (contains actual number)
        self.current_run = self.runs_dict[runs_display[-1]]
        self.current_deterministic_run = self.runs_dict[runs_display[-1]]

        # Update run selection (this will automatically trigger the callback)

        self.widgets["select_check_run_non_deterministic"].options = runs_display
        self.widgets["select_check_run_deterministic"].options = runs_display

        # If the run name happens to be the same, the callback has to be
        # triggered manually
        if runs_display[-1] == self.widgets["select_check_run_non_deterministic"].value:
            update_non_deterministic_run("value", runs_display[-1], runs_display[-1])
        # In any other case, updating the value is enough to trigger the
        # callback
        else:
            self.widgets["select_check_run_non_deterministic"].value = runs_display[-1]

        if runs_display[-1] == self.widgets["select_check_run_deterministic"].value:
            update_deterministic_run("value", runs_display[-1], runs_display[-1])
        else:
            self.widgets["select_check_run_deterministic"].value = runs_display[-1]

    def switch_view(self, run_name):
        """When received, switch selected run to run_name"""

        # This will trigger the widget's callback
        self.widgets["select_check_run"].value = run_name

        # Constructor

    def __init__(self, master, doc, data, deterministic_data, metadata):
        """
        Here are the most important attributes of the CompareRuns class

        master : reference to the ViewMaster class
        doc : an object provided by Bokeh to add elements to the HTML document
        data : pandas dataframe containing tests data (non-deterministic
        backends only)
        deterministic_data : pandas dataframe containing tests data (deterministic
        backends only)
        metadata : pandas dataframe containing all the tests metadata

        sources : ColumnDataSource object provided by Bokeh, contains current
        data (inside the .data attribute)
        widgets : dictionary of Bokeh widgets (no plots for this view)
        """

        self.master = master

        self.doc = doc
        self.data = data
        self.deterministic_data = deterministic_data
        self.metadata = metadata

        self.sources = {
            "deterministic": ColumnDataSource({}),
            "non_deterministic": ColumnDataSource({}),
        }

        self.widgets = {}

        # Setup Bokeh objects
        self.setup_widgets()

        # At this point, everything should have been initialized, so we can
        # show the data for the first time
        self.run_data.reset_index(inplace=True)
        self.deterministic_run_data.reset_index(inplace=True)

        if not self.run_data.empty:
            self.sources["non_deterministic"].data = self.run_data
        else:
            self.sources["non_deterministic"].data = {
                "check": [],
                "accuracy_threshold": [],
                "check_mode": [],
                "mu": [],
                "sigma": [],
            }

        if not self.deterministic_run_data.empty:
            self.sources["deterministic"].data = self.deterministic_run_data
        else:
            self.sources["deterministic"].data = {
                "check": [],
                "accuracy_threshold": [],
                "check_mode": [],
                "value": [],
                "reference_value": [],
            }
