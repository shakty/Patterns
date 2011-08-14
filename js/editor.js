window.onload = function() {
	

	var url = location.href.substring(0,location.href.lastIndexOf('edit/')) + 'validate';
		
	(function () {

	    var editor = CodeMirror.fromTextArea(document.getElementById("edit-content"), {
	        lineNumbers: true,
	        onKeyEvent: function(i, e) {
	          // Hook into F11
	          if ((e.keyCode == 122 || e.keyCode == 27) && e.type == 'keydown') {
	            e.stop();
	            return toggleFullscreenEditing();
	          }
	        },
	        onCursorActivity: function() {
		        editor.setLineClass(hlLine, null);
		        hlLine = editor.setLineClass(editor.getCursor().line, "activeline");
		    },
		    tabMode: "shift"
	    });

	    var hlLine = editor.setLineClass(0, "activeline");
	    
	    
	    var fullScreenNotice = 'Press F11 (or ESC in Safari on Mac OS X) when cursor is in the editor to toggle full screen editing.';
	    var noticeNode = document.createTextNode(fullScreenNotice);
	    document.getElementById("edit-content").parentNode.appendChild(noticeNode);
	    
	    document.getElementById('validate_pattern').onclick = function() {
	    	document.getElementById('validation_result').innerHTML = '...validating...';
	    	editor.save();
	    	jQuery.ajax({
	    		type: 'POST',
	    		url: url,
	    		data: 'pattern='+document.getElementById('edit-content').value,
	    		success: function ( data, status, xhr ) {
	    					document.getElementById('validation_result').innerHTML = '<strong>'+data+'</strong>';
    						//$('validation_result').replaceWith(data);
    					}
	    	});
	    }
	    
	    function toggleFullscreenEditing()
	    {
	        var editorDiv = jQuery('.CodeMirror-scroll');
	        if (!editorDiv.hasClass('fullscreen')) {
	            toggleFullscreenEditing.beforeFullscreen = { height: editorDiv.height(), width: editorDiv.width() }
	            editorDiv.addClass('fullscreen');
	            editorDiv.height('100%');
	            editorDiv.width('100%');
	            editor.refresh();
	        }
	        else {
	            editorDiv.removeClass('fullscreen');
	            editorDiv.height(toggleFullscreenEditing.beforeFullscreen.height);
	            editorDiv.width(toggleFullscreenEditing.beforeFullscreen.width);
	            editor.refresh();
	        }
	    }

	})();
	
};

//var myCodeMirror = CodeMirror(document.body);