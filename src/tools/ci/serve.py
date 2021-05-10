# Server for the Verificarlo CI report. This is simply a wrapper to avoid
# calling Bokeh directly.

import os


def serve(show, git_directory, git_url, port, allow_origin, logo_url):

    # Prepare arguments
    show = "--show" if show else ""

    git = ""
    if git_directory is not None:
        git = "git directory %s" % git_directory
    if git_url is not None:
        git = "git url %s" % git_url

    logo = ""
    if logo_url is not None:
        logo = "logo %s" % logo_url

    dirname = os.path.dirname(__file__)

    # Call the "bokeh serve" command on the system
    command = "bokeh serve %s/vfc_ci_report %s --allow-websocket-origin=%s:%s --port %s --args %s %s" \
        % (dirname, show, allow_origin, port, port, git, logo)

    os.system(command)
