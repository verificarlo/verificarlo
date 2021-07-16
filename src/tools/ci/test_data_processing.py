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

import verificarlo.sigdigits as sd
import scipy.stats
import numpy as np

# Magic numbers
# For the normality test :
min_pvalue = 0.05
probability = 0.9
confidence = 0.95


##########################################################################

def significant_digits(x):
    '''First wrapper to sd.significant_digits (returns results in base 2)'''

    if x.mu == 0:
        return 53

    # If the null hypothesis is rejected, call sigdigits with the General
    # formula:
    if x.pvalue < min_pvalue:
        # In a pandas DF, "values" actually refers to the array of columns, and
        # not the column named "values"
        distribution = x.values[0]
        distribution = distribution.reshape(len(distribution), 1)

        # The distribution's empirical average will be used as the reference
        mu = np.array([x.mu])

        s = sd.significant_digits(
            distribution,
            mu,
            precision=sd.Precision.Relative,
            method=sd.Method.General,

            probability=probability,
            confidence=confidence
        )

        # s is returned inside a list
        return s[0]

    # Else, manually compute sMCA (Stott-Parker formula)
    else:
        return min(-np.log2(np.absolute(x.sigma / x.mu)), 53)


def significant_digits_lower_bound(x):
    '''
    Second wrapper to sd.significant_digits : assumes that s2 has already been
    computed
    '''

    # If the null hypothesis is rejected, no lower bound
    if x.pvalue < min_pvalue:
        return x.s2

    elif x.mu == 0:
        return 53

    # Else, the lower bound will be computed with p= .9 alpha-1=.95
    else:
        distribution = x.values[0]
        distribution = distribution.reshape(len(distribution), 1)

        mu = np.array([x.mu])

        s = sd.significant_digits(
            distribution,
            mu,
            precision=sd.Precision.Relative,
            method=sd.Method.CNH,

            probability=0.9,
            confidence=0.95
        )
        return s[0]


def apply_data_pocessing(data):
    '''
    This function computes most test metrics (mu, sigma, quantiles, ...).
    Doesn't include significant digits.
    '''

    # Get empirical average, standard deviation and p-value
    data["mu"] = np.average(data["values"])
    data["sigma"] = np.std(data["values"])
    data["pvalue"] = scipy.stats.shapiro(data["values"]).pvalue

    # Quantiles
    data["min"] = np.min(data["values"])
    data["quantile25"] = np.quantile(data["values"], 0.25)
    data["quantile50"] = np.quantile(data["values"], 0.50)
    data["quantile75"] = np.quantile(data["values"], 0.75)
    data["max"] = np.max(data["values"])

    # Assert validation

    if data["assert_mode"] == "absolute":
        data["assert"] = True if data["sigma"] < abs(
            data["accuracy_threshold"]) else False

    elif data["assert_mode"] == "relative":
        data["assert"] = True if abs(
            data["sigma"] /
            data["mu"]) < abs(data["accuracy_threshold"]) else False

    else:
        data["assert"] = True

    return data


def data_processing(data):
    '''
    Computes all metrics on the dataframe
    '''

    # Converts classic lists to Numpy arrays
    data["values"] = data["values"].apply(lambda x: np.array(x))

    # Computes most of test metrics
    data = data.apply(apply_data_pocessing, axis=1)

    # Significant digits
    data["s2"] = data.apply(significant_digits, axis=1)
    data["s10"] = data["s2"].apply(lambda x: sd.change_base(x, 10))

    # Lower bound of the confidence interval using the sigdigits module
    data["s2_lower_bound"] = data.apply(significant_digits_lower_bound, axis=1)
    data["s10_lower_bound"] = data["s2_lower_bound"].apply(
        lambda x: sd.change_base(x, 10))

    data["nsamples"] = data["values"].apply(len)

    return data


def validate_deterministic_probe(x):
    '''
    This function will be applied to results dataframes of deterministic
    backends to validate probes depending on if they are absolute or relative
    '''

    if x["assert_mode"] == "absolute":
        return True if abs(
            x["value"] -
            x["reference_value"]) < abs(x["accuracy_threshold"]) else False

    if x["assert_mode"] == "relative":
        return True if abs(x["value"] - x["reference_value"]) / \
            abs(x["reference_value"]) < abs(x["accuracy_threshold"]) else False

    return True
