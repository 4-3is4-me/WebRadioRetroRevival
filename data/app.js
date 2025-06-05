// app.js
function getData() {
  const name = document.getElementById("name").value;
  const countrycode = document.getElementById("countrycode").value;

  const servers = [
    "https://all.api.radio-browser.info",
    "https://de1.api.radio-browser.info",
    "https://de2.api.radio-browser.info",
    "https://fi1.api.radio-browser.info"
  ];

  function fetchServers(index) {
    if (index >= servers.length) {
      document.getElementById("result").innerHTML = `All servers failed to respond. Please try later.`;
      return;
    }

    fetch(`${servers[index]}/json/servers`)
      .then(response => {
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        return response.json();
      })
      .then(hosts => {
        // console.log(hosts);
        fetchStations(hosts, name, countrycode);
      })
      .catch(error => {
        console.log(`Error occurred with server ${servers[index]}: `, error);
        // Try the next server
        fetchServers(index + 1);
      });
  }

  function fetchStations(hosts, name, countrycode) {
    // Select a random host from the list of servers
    let ourhost = hosts[Math.floor(Math.random() * hosts.length)];
    const url = `https://${ourhost.name}/json/stations/search?name=${name}&countrycode=${countrycode}&limit=10`;

    fetch(url)
      .then(response => {
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        return response.json();
      })
      .then(data => {
        document.getElementById("result").innerHTML = ''; // clears the previous results
        if (data.length === 0) {
          document.getElementById("result").innerHTML = 'No results found';
        } else {
          data.forEach(station => {
            const stationName = document.createElement("p");
            stationName.innerHTML = `<strong>Name:</strong> ${station.name}<br><strong>URL:</strong> ${station.url}`;
            document.getElementById("result").appendChild(stationName);
          });
        }
      })
      .catch(error => {
        console.log("Error fetching stations: ", error);
        document.getElementById("result").innerHTML = `Error: ${error.message}`;
        fetchStations(hosts, name, countrycode)
      });
  }

  // Start fetching from the first server
  fetchServers(0);
}


function openTab(evt, tabName) {
  var i, tabcontent, tablinks;
  tabcontent = document.getElementsByClassName("tab");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }
  tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
      tablinks[i].className = tablinks[i].className.replace(" active", "");
    }
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}
