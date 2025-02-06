const topLeft = document.getElementById("newb");
const topRight = document.getElementById("returning");
var r = document.querySelector(':root');

topLeftClick = function() {
    console.log("new user!!");

    window.location.href = "/screenside";
}

topRightClick = function() {

    console.log("existing user!!");

    window.location.href = "/screenside";
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
