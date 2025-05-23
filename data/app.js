// app.js
function getData() {
  const name = document.getElementById("name").value;
  const countrycode = document.getElementById("countrycode").value;

  // picking a server as per API guidance  https://api.radio-browser.info/examples/serverlist-browser.js
  try {
    fetch(`https://de1.api.radio-browser.info/json/servers`)
      .then(response => response.json())
      .then(hosts =>  {
        let ourhost = hosts[Math.floor(Math.random() * hosts.length)];
        const url = `https://${ourhost.name}/json/stations/search?name=${name}&countrycode=${countrycode}&limit=10`;
        fetch(url)
          .then(response => response.json())
          .then(data => {
            document.getElementById("result").innerHTML = ''; // clears the previous results
            if (data.length === 0) {
              document.getElementById("result").innerHTML = 'No results found';
            } else {
              data.forEach(station => {
                const stationName = document.createElement("p");
                stationName.innerHTML = `<strong>Name:</strong> ${station.name}<br><strong>URL:</strong> ${station.url}<br>`;
                document.getElementById("result").appendChild(stationName);
              });
            }
          });
      });
    } catch (error)  {
        console.log(error);
        document.getElementById("result").innerHTML = `Error: ${error}`;
    }
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
