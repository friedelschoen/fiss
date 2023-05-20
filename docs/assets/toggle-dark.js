function toggle_dark() {
	var githubImage = document.getElementById('github');
	var toggleButton = document.getElementById('toggle_dark');
	if (document.body.classList.toggle('dark')) {
		// is dark
		githubImage.src = 'assets/github-mark-white.svg';
		toggleButton.innerHTML = ' turn the lights on ';
	} else {
		// is light
		githubImage.src = 'assets/github-mark.svg';
		toggleButton.innerHTML = ' turn the lights off ';
	}
}

document.addEventListener('readystatechange', function (state) {
	if (document.readyState == 'complete')
		if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
			toggle_dark();
		}
});
