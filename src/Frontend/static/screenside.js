const topLeft = document.getElementById("tl");
const topRight = document.getElementById("tr");
var r = document.querySelector(':root');

function sendData(value) {
    var data = value;
    fetch('/eyetrackingside', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({data: data})
      })
      .then(response => response.text())
      .then(result => {
        console.log(result);
      })
      .catch(error => {
        console.error('Error:', error);
      });
}
topLeftClick = function() {
    console.log("screen on left!");
    sendData(0);
    window.location.href = "/setup";
}

topRightClick = function() {
    sendData(1);
    console.log("screen on right!");

    window.location.href = "/setup";
}

function toggleFullScreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen();
    } else if (document.exitFullscreen) {
      document.exitFullscreen();
    }
  }
  document.addEventListener(
    "keydown",
    (e) => {
      if (e.key === "Enter") {
        toggleFullScreen();
      }
    },
    false,
  );


topLeft.addEventListener("click", topLeftClick);
topRight.addEventListener("click", topRightClick);
