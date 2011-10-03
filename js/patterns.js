window.onload = function() {

	(function ($) {
	
	  Drupal.behaviors.patternsBehavior = {
	    attach: function(context, settings) {
		  $('.pattern-info').hide();
		  $('.pattern-info').css({'position' : 'absolute', 'width' : '300px'});
		  $('.pattern-title').hover(
		    function(){
		      $('#' + $(this).attr('id') + '-info').show();
		    },
		    function(){
		      $('#' + $(this).attr('id') + '-info').hide();
		    }
		  );
	    }
	  };
	})(jQuery);
	
  	var url = location.href.substring(0,location.href.lastIndexOf('admin/patterns/') + 'admin/patterns/'.length) + 'refresh';
	
	var refresh = document.getElementById("refresh-all");
	
	if (refresh) {	
		refresh.onclick = function() {
			document.getElementById('all_patterns_div').innerHTML = '...refreshing...';
			
			jQuery.ajax({
				type: 'POST',
				url: url,
				// TODO: This needs to be encoded.
				//data: 'pattern=' + textarea.value,
				success: function ( data, status, xhr ) {
							document.getElementById('all_patterns_div').innerHTML = data;
							//$('all_patterns_div').replaceWith(data);
						}
			});
		};
	}
	else{
		alert('NO');
	}
}