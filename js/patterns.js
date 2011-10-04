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
		
	  	var url = location.href.substring(0,location.href.lastIndexOf('admin/patterns') + 'admin/patterns'.length) + '/refresh';
		
		var refresh = document.getElementById("refresh-all");
		
		if (refresh) {	
			refresh.onclick = function() {
				document.getElementById('all_patterns_div').innerHTML = '...refreshing...';
				
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