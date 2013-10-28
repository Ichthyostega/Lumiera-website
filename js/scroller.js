function pageScrollRec(current_position) {
    window.scrollBy(0, 1); // horizontal and vertical scroll increment
    if (current_position < window.pageYOffset) {
      setTimeout('pageScrollRec(window.pageYOffset)', 50); // scrolls every 100 milliseconds
    }
}

function pageScroll() {
    setTimeout('pageScrollRec(0)', 1000);
}
