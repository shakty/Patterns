
Drupal.behaviors.patternsBehavior = function (context) {
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
};