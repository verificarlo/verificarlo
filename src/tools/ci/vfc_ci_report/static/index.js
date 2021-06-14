
// Modify the top-right button link to the repository
function changeRepository(repoName) {

    gitRepoLink = document.getElementById("git-repo-link");

    repoUrl = repoNamesDict[repoName];

    // If we're lookin at commits not linked to a repo
    if(repoUrl == "") {
        // Hide the link button
        gitRepoLink.style.display = "none";
    }

    // We have selected a valid Git repo
    else {
        // Show the link button
        gitRepoLink.style.display = "";

        // Get the name alone (without owner/branch)
        name = repoName.split(/\/|:/)[1];

        // Get host name (GitHub/GitLab)
        let gitHost = "";
        if(repoUrl.indexOf("github.com") != -1) {
            gitHost = "GitHub";
        }
        else {
            gitHost = "GitLab";
        }

        // Update the button
        gitRepoLink.href = repoUrl;
        gitRepoLink.innerHTML = name + " on " + gitHost;
    }
}



// Helper function to generate the link to a git commit


// Listen to clicks on breadcrumb (for responsive header)
document.getElementById("navbar-burger")
.addEventListener("click", () => {

    document.getElementById("navbar-burger")
    .classList.toggle("is-active");

    document.getElementById("navbar-content")
    .classList.toggle("is-active");

})



// Helper function to navigate between views
function changeView(classPrefix) {

    // Enable/disable the active class on buttons
    let buttons = document.getElementById("buttons-container").childNodes;
    let toggledButtonId = classPrefix + "-button";

    for(let i=0; i<buttons.length; i++) {
        if(toggledButtonId == buttons[i].id) {
            buttons[i].classList.add("is-active");
        }
        else if(buttons[i].classList != undefined) {
            buttons[i].classList.remove("is-active");
        }
    }

    // Show hide the containers
    let containers = document.getElementsByTagName("MAIN");
    let toggledContainerId = classPrefix + "-container"

    for(let i=0; i<containers.length; i++) {
        if(toggledContainerId == containers[i].id) {
            containers[i].style.display = "block";
        }
        else {
            containers[i].style.display = "none";
        }
    }
}

// Listen to clicks on "Compare runs" button
document.getElementById("compare-runs-button").addEventListener("click", () => {
    // Nothing else to do for this button
    changeView("compare-runs");
});

// Listen to clicks on "Inspect runs" button
// (dedicated function as this needs to be called in a CustomJS callback)
function goToInspectRuns() {
    window.scrollTo(0, 0);

    changeView("inspect-runs");
}

document.getElementById("inspect-runs-button")
.addEventListener("click", goToInspectRuns);



// Toggle the display properties of the loader/report
function removeLoader() {
    document.getElementById("loader")
    .style.display = "none";

    document.getElementById("report")
    .style.display = "";
}

// To detect the end of Bokeh initialization and remove the loader,
// we look at the number of children of a div containing widgets
let nChildren = document.getElementById('compare-widgets')
.getElementsByTagName('*').length;

function pollBokehLoading() {
    let newNChildren = document.getElementById('compare-widgets')
    .getElementsByTagName('*').length;

    if(newNChildren != nChildren) {
        removeLoader();
    }
    else {
        setTimeout(pollBokehLoading, 100);
    }
}
setTimeout(pollBokehLoading, 100);



// Update the run metadata (in inspect run mode)
function updateRunMetadata(runId) {

    // Assume runId is the run's timestamp
    let run = metadata[runId];

    // If it is undefined, perform a search by name
    // (by iterating metadata)
    if(!run) {
        for(let [key, value] of Object.entries(metadata)) {

            if (!metadata.hasOwnProperty(key)) continue;
            if(value.name == runId) {
                run = value;
                break;
            }
        }
    }


    // At this point, run is correctly set

    // Generate the commit link and host name
    let commit_link = "";
    let gitHost = "";
    if(run.remote_url.indexOf("github.com") != -1) {
        commit_link = run.remote_url + "/commit/" + run.hash;
        gitHost = "GitHub";
    } else {
        commit_link = run.remote_url + "/-/commit/" + run.hash;
        gitHost = "GitLab";
    }


    // Edit innerHTML with new metadata
    document.getElementById("run-date").innerHTML = run.date;

    if(run.is_git_commit) {
        document.getElementById("is-git-commit").style.display = "";
        document.getElementById("not-git-commit").style.display = "none";

        document.getElementById("run-hash").innerHTML = run.hash;
        document.getElementById("run-author").innerHTML = run.author;
        document.getElementById("run-message").innerHTML = run.message;

        document.getElementById("git-commit-link")
        .setAttribute("href", commit_link);
        document.getElementById("git-commit-link")
        .innerHTML = "View this commit on " + gitHost;

    } else {
        document.getElementById("is-git-commit").style.display = "none";
        document.getElementById("not-git-commit").style.display = "";

        document.getElementById("run-hash").innerHTML = "";
        document.getElementById("run-author").innerHTML = "";
        document.getElementById("run-message").innerHTML = "";
    }
}
