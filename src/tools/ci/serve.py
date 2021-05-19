# Server for the Verificarlo CI report. This is simply a wrapper to avoid
# calling Bokeh directly.

import os


# Entry point of vfc_ci serve
def run(directory, show, port, allow_origin, logo_url):

    # Prepare arguments
    directory = "directory %s" % directory

    show = "--show" if show else ""

    logo = ""
    if logo_url is not None:
        logo = "logo %s" % logo_url

    dirname = os.path.dirname(__file__)

    # Call the "bokeh serve" command on the system
    command = "bokeh serve %s/vfc_ci_report %s --allow-websocket-origin=%s:%s --port %s --args %s %s" \
        % (dirname, show, allow_origin, port, port, directory, logo)

    os.system(command)
