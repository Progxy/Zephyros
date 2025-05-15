document.addEventListener("DOMContentLoaded", (event) => {

	const keyInput = document.getElementById('keyInput');
	const valueInput = document.getElementById('valueInput');
	const addBtn = document.getElementById('addBtn');
	const sendBtn = document.getElementById('sendBtn');
	const visualizer = document.getElementById('visualizer');

	let data = {};

	function refreshVisualizer() {
	  visualizer.innerHTML = '';
	  for (const [key, value] of Object.entries(data)) {
		const pairDiv = document.createElement('div');
		pairDiv.className = 'pair';

		const textSpan = document.createElement('span');
		textSpan.textContent = `${key}: ${value}`;

		const delBtn = document.createElement('button');
		delBtn.textContent = 'Delete';
		delBtn.onclick = () => {
		  delete data[key];
		  refreshVisualizer();
		};

		pairDiv.appendChild(textSpan);
		pairDiv.appendChild(delBtn);
		visualizer.appendChild(pairDiv);
	  }
	}

	addBtn.onclick = () => {
	  const key = keyInput.value.trim();
	  const value = valueInput.value.trim();
	  if (key && value) {
		data[key] = value;
		keyInput.value = '';
		valueInput.value = '';
		refreshVisualizer();
	  } else {
		alert('Please enter both key and value');
	  }
	};

	sendBtn.onclick = () => {
	  fetch('http://localhost:6969', {
		method: 'POST',
		headers: {
		  'Content-Type': 'application/json'
		},
		body: JSON.stringify(data)
	  })
	  .then(response => response.text())
	  .then(result => {
		alert('Server response: ' + result);
	  })
	  .catch(error => {
		alert('Error: ' + error);
	  });
	};

});


