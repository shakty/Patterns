
$(document).ready(function(){
  $('.pattern-info').hide();
  $('.pattern-title').click(
    function(){
      $('#' + $(this).attr('id') + '-info').toggle();
      return false;
    }
  );
});
