jQuery('document').ready( function() {

	(function(){
		var url = location.href.substring(0,location.href.lastIndexOf('admin/patterns/') + 'admin/patterns/'.length) + 'validate';
		
		var textarea = document.getElementById("edit-content");
		alert(typeof(textarea));
		if (textarea) {
			alert('3');
			document.getElementById('validate_pattern').onclick = function() {
				document.getElementById('validation_result').innerHTML = '...validating...';
				
				// CO2editor is defined by editor.js (if loaded)		
				if (typeof(CO2editor) !== "undefined") {
					CO2editor.save();
				}
				
				jQuery.ajax({
					type: 'POST',
					url: url,
					data: 'pattern='+document.getElementById('edit-content').value,
					success: function ( data, status, xhr ) {
								document.getElementById('validation_result').innerHTML = '<strong>'+data+'</strong>';
								//$('validation_result').replaceWith(data);
							}
				});
			};
		}
	})();
		
	
});