; Core version
; ------------
core = 7.x
  
; API version
; ------------
api = 2
  
; Core project
; ------------
; Drupal 7.x. Requires the `core` property to be set to 7.x.
projects[drupal][version] = 7

  
  
; Modules
; --------
projects[] = token
projects[] = libraries

projects[patterns][type] = "module"
projects[patterns][download][type] = "git"
projects[patterns][download][url] = "http://git.drupal.org/project/patterns.git"
projects[patterns][download][branch] = "7.x-1.x"

; Libraries
; ---------
libraries[spyc][download][type] = "get"
libraries[spyc][download][url]= "http://spyc.googlecode.com/files/spyc-0.5.zip"
libraries[spyc][directory_name] = "spyc"
libraries[spyc][destination] = "libraries"

libraries[codemirror][download][type] = "get"
libraries[codemirror][download][url]= "http://codemirror.net/codemirror.zip"
libraries[codemirror][directory_name] = "codemirror"
libraries[codemirror][destination] = "libraries"
