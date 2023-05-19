function toggle_dark() {
	var githubImage = document.getElementById('github');
	var toggleButton = document.getElementById('toggle_dark');
	if (document.body.classList.toggle('dark')) {
		// is dark
		githubImage.src = 'assets/github-mark-white.svg';
		toggleButton.innerHTML = ' licht aandoen ';
	} else {
		// is light
		githubImage.src = 'assets/github-mark.svg';
		toggleButton.innerHTML = ' licht uitdoen ';
	}
}
