const topLeft = document.getElementById("tl");
const topRight = document.getElementById("tr");
var r = document.querySelector(':root');

/*function sendData(value) {
    $.ajax({
        url: '/eyetrackingside',
        type: 'POST',
        data: { 'data': value },
        success: function(response) {
            document.getElementById('output').innerHTML = response;
        },
        error: function(error) {
            console.log(error);
        }
    });
}*/
topLeftClick = function() {
    console.log("screen on left!");
    //sendData(0);
    window.location.href = "/setup";
}

topRightClick = function() {
    //sendData(1);
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
