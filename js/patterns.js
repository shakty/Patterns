window.onload = function() {
	
	(function () {
		  
		jQuery('.pattern-info').hide();
		jQuery('.pattern-info').css({'position' : 'absolute', 'width' : '300px'});
		jQuery('.pattern-title').hover(
		    function(){
		    	jQuery('#' + jQuery(this).attr('id') + '-info').show();
		    },
		    function(){
		    	jQuery('#' + jQuery(this).attr('id') + '-info').hide();
		    }
		  );
		
		var detect = 'admin/patterns';
		var idx = -1;
		idx = unescape(location.href).lastIndexOf(detect);
		var url = unescape(location.href).substring(0,idx + detect.length) + '/refresh';
		
		var refresh = document.getElementById("refresh-all");
		
		if (refresh) {	
			refresh.onclick = function() {
				if (idx === -1) {
					alert(Drupal.t('Patterns could not recognize the url of the website. Cannot refresh.'));
					return;
				}
				var allp_divs = document.getElementById('all_patterns_div');
				allp_divs.parentNode.removeChild(document.getElementById('removed_patterns_div'));
				allp_divs.innerHTML = '...refreshing...';
				
				jQuery.ajax({
					type: 'POST',
					url: url,
					// TODO: This needs to be encoded.
					success: function ( data, status, xhr ) {
								document.getElementById('all_patterns_div').innerHTML = data;
							}
				});
			};
		}
	
	
	})();
}