document.getElementById('uploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    var fileInput = document.getElementById('update_file');
    var file = fileInput.files[0];
    var updateType = document.getElementById('update_type').value;
    var statusDiv = document.getElementById('status');
    var progressBar = document.getElementById('progress-bar');
    var progressContainer = document.getElementById('progress-container');

    if (!file) {
        statusDiv.innerHTML = 'Please select a file!';
        statusDiv.className = 'message error';
        return;
    }

    var formData = new FormData();
    formData.append('update', file, file.name);

    var xhr = new XMLHttpRequest();
    var uploadUrl = (updateType === 'firmware') ? '/update/firmware' : '/update/data';

    xhr.open('POST', uploadUrl, true);

    xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
            var percentComplete = (e.loaded / e.total) * 100;
            progressBar.style.width = percentComplete + '%';
            progressBar.innerHTML = Math.round(percentComplete) + '%';
        }
    };

    xhr.onloadstart = function() {
        document.getElementById('uploadForm').classList.add('hidden');
        progressContainer.classList.remove('hidden');
        statusDiv.innerHTML = '';
        statusDiv.className = 'message';
        progressBar.style.width = '0%';
        progressBar.innerHTML = '0%';
    };

    xhr.onload = function() {
        if (xhr.status === 200) {
            statusDiv.innerHTML = 'Upload complete! Rebooting...';
            statusDiv.className = 'message success';
        } else {
            statusDiv.innerHTML = 'Upload failed! ' + xhr.responseText;
            statusDiv.className = 'message error';
            document.getElementById('uploadForm').classList.remove('hidden');
            progressContainer.classList.add('hidden');
        }
    };

    xhr.onerror = function() {
        statusDiv.innerHTML = 'Upload failed! A network error occurred.';
        statusDiv.className = 'message error';
        document.getElementById('uploadForm').classList.remove('hidden');
        progressContainer.classList.add('hidden');
    };

    xhr.send(formData);
});