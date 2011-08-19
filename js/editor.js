window.onload = function() {
	
	(function () {
		
		var url = location.href.substring(0,location.href.lastIndexOf('edit/')) + 'validate';

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
	    
	    function toggleFullscreenEditing () {
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
	    
	    window.CO2editor = editor;
	    
	})();
	
};