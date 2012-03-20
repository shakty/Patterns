jQuery('document').ready( function() {

	(function(){
		
		var detect = 'admin/patterns';
		var idx = -1;
		idx = unescape(location.href).lastIndexOf(detect);
		
		var url = unescape(location.href).substring(0,idx + detect.length) + '/validate';
		
		function validate(textarea, format_selector) {
			if (!textarea) return;
		
			var vp = document.getElementById('validate_pattern');
			
			if (vp) {
				vp.onclick = function() {
					
					if (idx === -1) {
						document.getElementById('validation_result').innerHTML = '<strong>Patterns could not recognize the url of the web-site. Validation is disabled.</strong>';
						return;
					}
						
					document.getElementById('validation_result').innerHTML = '...validating...';
					
					// CO2editor is defined by editor.js (if loaded)		
					if (typeof(CO2editor) !== "undefined") {
						CO2editor.save();
					}
					
					var format = (format_selector && format_selector.value) ? format_selector.value
																			: 'unknown';
					
					jQuery.ajax({
						type: 'POST',
						url: url,
						// TODO: This needs to be encoded.
						//data: 'pattern=' + textarea.value + '&format=' + format,
						data: {pattern: textarea.value,
							   format: format
						},
						success: function ( data, status, xhr ) {
									document.getElementById('validation_result').innerHTML = '<strong>'+data+'</strong>';
									//$('validation_result').replaceWith(data);
								}
					});
				};
			}
		}
		
		validate(document.getElementById("edit-content-db"),document.getElementById("edit-format-db"));
		validate(document.getElementById("edit-content"),document.getElementById("edit-format"));
		
		
	})();
		
	
});