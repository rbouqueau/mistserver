const char *embed_js = 
  "function mistembed(streamname) {\n  // return the current flash version\n  funct" \
  "ion flash_version() {\n    var version = 0;\n\n    try {\n      // check in the " \
  "mimeTypes\n      version = navigator.mimeTypes['application/x-shockwave-flash']." \
  "enabledPlugin.description.replace(/([^0-9\\.])/g, '').split('.')[0];\n    } catc" \
  "h(e){}\n    try {\n      // for our special friend IE\n      version = new Activ" \
  "eXObject('ShockwaveFlash.ShockwaveFlash').GetVariable(\"$version\").replace(/([^" \
  "0-9\\,])/g, '').split(',')[0];\n    } catch(e){}\n\n    return parseInt(version," \
  " 10);\n  };\n\n  // return true if silverlight is installed\n  function silverli" \
  "ght_installed() {\n    var plugin;\n    \n    try {\n      // check in the mimeT" \
  "ypes\n      plugin = navigator.plugins[\"Silverlight Plug-In\"];\n      return !" \
  "!plugin;\n    } catch(e){}\n    try {\n      // for our special friend IE\n     " \
  " plugin = new ActiveXObject('AgControl.AgControl');\n      return true;\n    } c" \
  "atch(e){}\n\n    return false;\n  };\n\n  // return true if the browser thinks i" \
  "t can play the mimetype\n  function html5_video_type(type) {\n    var support = " \
  "false;\n\n    try {\n      var v = document.createElement('video');\n\n      if(" \
  " v && v.canPlayType(type) != \"\" )\n      {\n        support = true; // true-is" \
  "h, anyway\n      }\n    } catch(e){}\n\n    return support;\n  }\n  \n  //return" \
  " true if rtsp is supported\n  function rtsp_support() {\n    var plugin;\n    \n" \
  "    try {\n      // check in the mimeTypes\n      plugin = navigator.mimeTypes[\"" \
  "application/x-google-vlc-plugin\"];\n      return !!plugin;\n    } catch(e){}\n " \
  "   try {\n      // for our special friend IE\n      plugin = new ActiveXObject('" \
  "VideoLAN.Vlcplugin.1');\n      return true;\n    } catch(e){}\n    \n    return " \
  "false;\n  }\n\n  // parse a \"type\" string from the controller. Format:\n  // x" \
  "xx/# (e.g. flash/3) or xxx/xxx/xxx (e.g. html5/application/ogg)\n  function pars" \
  "eType(type) {\n    var split = type.split('/');\n    \n    if( split.length > 2 " \
  ") {\n      split[1] += '/' + split[2];\n    }\n    \n    return split;\n  }\n  \n" \
  "  // return true if a type is supported\n  function hasSupport(type) {\n    var " \
  "typemime = parseType(type);\n    \n    switch(typemime[0]) {\n      case 'flash'" \
  ":             return flash_version() >= parseInt(typemime[1], 10);            br" \
  "eak;\n      case 'html5':             return html5_video_type(typemime[1]);     " \
  "                      break;\n      case 'rtsp':              return rtsp_suppor" \
  "t();                                          break;\n      case 'silverlight': " \
  "      return silverlight_installed();                                 break;\n  " \
  "    default:                  return false;                                     " \
  "              break;\n    }\n  }\n\n  \n  // build HTML for certain kinds of typ" \
  "es\n  function buildPlayer(src, container, videowidth, videoheight, vtype) {\n  " \
  "  // used to recalculate the width/height\n    var ratio;\n\n    // get the cont" \
  "ainer's width/height\n    var containerwidth = parseInt(container.scrollWidth, 1" \
  "0);\n    var containerheight = parseInt(container.scrollHeight, 10);\n\n    if(v" \
  "ideowidth > containerwidth && containerwidth > 0) {\n      ratio = videowidth / " \
  "containerwidth;\n\n      videowidth /= ratio;\n      videoheight /= ratio;\n    " \
  "}\n\n    if(videoheight > containerheight && containerheight > 0) {\n      ratio" \
  " = videoheight / containerheight;\n\n      videowidth /= ratio;\n      videoheig" \
  "ht /= ratio;\n    }\n\n    var maintype = parseType(src.type);\n    mistvideo[st" \
  "reamname].embedded = src;\n    \n    switch(maintype[0]) {\n      case 'flash':\n" \
  "        // maintype[1] is already checked (i.e. user has version > maintype[1])\n" \
  "        var flashplayer,\n        url = encodeURIComponent(src.url) + '&controlB" \
  "arMode=floating&initialBufferTime=0.5&expandedBufferTime=5&minContinuousPlayback" \
  "Time=3' + (vtype == 'live' ? \"&streamType=live\" : \"\") + (autoplay ? '&autoPl" \
  "ay=true' : '');\n        \n        if( parseInt(maintype[1], 10) >= 10 ) {\n    " \
  "      flashplayer = 'http://fpdownload.adobe.com/strobe/FlashMediaPlayback_101.s" \
  "wf';\n        }\n        else {\n          flashplayer = 'http://fpdownload.adob" \
  "e.com/strobe/FlashMediaPlayback.swf';\n        }\n        \n        container.in" \
  "nerHTML += '<object width=\"' + videowidth + '\" height=\"' + videoheight + '\">" \
  "' +\n                                '<param name=\"movie\" value=\"' + flashpla" \
  "yer + '\"></param>' + \n                                '<param name=\"flashvars" \
  "\" value=\"src=' + url + '\"></param>' +\n                                '<para" \
  "m name=\"allowFullScreen\" value=\"true\"></param>' +\n                         " \
  "       '<param name=\"allowscriptaccess\" value=\"always\"></param>' + \n       " \
  "                         '<param name=\"wmode\" value=\"direct\"></param>' +\n  " \
  "                              (autoplay ? '<param name=\"autoPlay\" value=\"true" \
  "\">' : '') +\n                                '<embed src=\"' + flashplayer + '\"" \
  " type=\"application/x-shockwave-flash\" allowscriptaccess=\"always\" allowfullsc" \
  "reen=\"true\" width=\"' + videowidth + '\" height=\"' + videoheight + '\" flashv" \
  "ars=\"src=' + url + '\"></embed>' + \n                              '</object>';" \
  "\n      break;\n\n      case 'html5':\n        container.innerHTML += '<video wi" \
  "dth=\"' + videowidth + '\" height=\"' + videoheight + '\" src=\"' + encodeURI(sr" \
  "c.url) + '\" controls=\"controls\" '+(autoplay ? 'autoplay=\"autoplay\"' : '')+'" \
  "><strong>No HTML5 video support</strong></video>';\n        break;\n        \n  " \
  "    case 'rtsp':\n        /*container.innerHTML += '<object classid=\"clsid:CFCD" \
  "AA03-8BE4-11cf-B84B-0020AFBBCCFA\" width=\"'+videowidth+'\" height=\"'+videoheig" \
  "ht+'\">'+\n                                  '<param name=\"src\" value=\"'+enco" \
  "deURI(src.url)+'\">'+\n                                  '<param name=\"console\"" \
  " value=\"video1\">'+\n                                  '<param name=\"controls\"" \
  " value=\"All\">'+\n                                  '<param name=\"autostart\" " \
  "value=\"false\">'+\n                                  '<param name=\"loop\" valu" \
  "e=\"false\">'+\n                                  '<embed name=\"myMovie\" src=\"" \
  "'+encodeURI(src.url)+'\" width=\"'+videowidth+'\" height=\"'+videoheight+'\" aut" \
  "ostart=\"false\" loop=\"false\" nojava=\"true\" console=\"video1\" controls=\"Al" \
  "l\"></embed>'+\n                                  '<noembed>Something went wrong" \
  ".</noembed>'+\n                                '</object>'; //realplayer, doesnt" \
  " work */\n        container.innerHTML +=  '<embed type=\"application/x-google-vl" \
  "c-plugin\"'+\n                                  'pluginspage=\"http://www.videol" \
  "an.org\"'+\n                                  'width=\"'+videowidth+'\"'+\n     " \
  "                             'height=\"'+videoheight+'\"'+\n                    " \
  "              'target=\"'+encodeURI(src.url)+'\"'+\n                            " \
  "      'autoplay=\"'+(autoplay ? 'yes' : 'no')+'\"'+\n                           " \
  "     '>'+\n                                '</embed>'+\n                        " \
  "        '<object classid=\"clsid:9BE31822-FDAD-461B-AD51-BE1D1C159921\" codebase" \
  "=\"http://downloads.videolan.org/pub/videolan/vlc/latest/win32/axvlc.cab\">'+\n " \
  "                               '</object>'; //vlc, seems to work, sort of. it's " \
  "trying anyway\n      break;\n        \n      case 'silverlight':\n        contai" \
  "ner.innerHTML +=  '<object data=\"data:application/x-silverlight,\" type=\"appli" \
  "cation/x-silverlight\" width=\"' + videowidth + '\" height=\"' + videoheight + '" \
  "\">'+\n                                  '<param name=\"source\" value=\"' + enc" \
  "odeURI(src.url) + '/player.xap\"/>'+\n                                  '<param " \
  "name=\"onerror\" value=\"onSilverlightError\" />'+\n                            " \
  "      '<param name=\"autoUpgrade\" value=\"true\" />'+\n                        " \
  "          '<param name=\"background\" value=\"white\" />'+\n                    " \
  "              '<param name=\"enableHtmlAccess\" value=\"true\" />'+\n           " \
  "                       '<param name=\"minRuntimeVersion\" value=\"3.0.40624.0\" " \
  "/>'+\n                                  '<param name=\"initparams\" value =\\'au" \
  "toload=false,'+(autoplay ? 'autoplay=true' : 'autoplay=false')+',displaytimecode" \
  "=false,enablecaptions=true,joinLive=true,muted=false,playlist=<playList><playLis" \
  "tItems><playListItem title=\"Test\" description=\"testing\" mediaSource=\"' + en" \
  "codeURI(src.url) + '\" adaptiveStreaming=\"true\" thumbSource=\"\" frameRate=\"2" \
  "5.0\" width=\"\" height=\"\"></playListItem></playListItems></playList>\\' />'+\n" \
  "                                  '<a href=\"http://go.microsoft.com/fwlink/?Lin" \
  "kID=124807\" style=\"text-decoration: none;\"> <img src=\"http://go.microsoft.co" \
  "m/fwlink/?LinkId=108181\" alt=\"Get Microsoft Silverlight\" style=\"border-style" \
  ": none\" /></a>'+\n                                '</object>';\n      break;\n " \
  "     default:\n        container.innerHTML += '<strong>Missing embed code for ou" \
  "tput type \"'+src.type+'\"</strong>';\n        video.error = 'Missing embed code" \
  " for output type \"'+src.type;\n    }\n  }\n  \n  var video = mistvideo[streamna" \
  "me],\n  container = document.createElement('div'),\n  scripts = document.getElem" \
  "entsByTagName('script'),\n  me = scripts[scripts.length - 1];\n  \n  if (me.pare" \
  "ntNode.hasAttribute('data-forcetype')) {\n    var forceType = me.parentNode.getA" \
  "ttribute('data-forcetype');\n  }\n  if (me.parentNode.hasAttribute('data-forcesu" \
  "pportcheck')) {\n    var forceSupportCheck = true;\n  }\n  if (me.parentNode.has" \
  "Attribute('data-autoplay')) {\n    var autoplay = true;\n  }\n  \n  if (video.wi" \
  "dth == 0) { video.width = 250; }\n  if (video.height == 0) { video.height = 250;" \
  " }\n  \n  // create the container\n  me.parentNode.insertBefore(container, me);\n" \
  "  // set the class to 'mistvideo'\n  container.setAttribute('class', 'mistvideo'" \
  ");\n  // remove script tag\n  me.parentNode.removeChild(me);\n\n  if(video.error" \
  ") {\n    // there was an error; display it\n    container.innerHTML = ['<strong>" \
  "Error: ', video.error, '</strong>'].join('');\n  }\n  else if ((typeof video.sou" \
  "rce == 'undefined') || (video.source.length < 1)) {\n    // no stream sources\n " \
  "   container.innerHTML = '<strong>Error: no protocols found</strong>';\n  }\n  e" \
  "lse {\n    // no error, and sources found. Check the video types and output the " \
  "best\n    // available video player.\n    var i,\n      vtype = (video.type ? vi" \
  "deo.type : 'unknown'),\n      foundPlayer = false,\n      len = video.source.len" \
  "gth;\n      \n    for (var i in video.source) {\n      var support = hasSupport(" \
  "video.source[i].type);\n      video.source[i].browser_support = support;\n      " \
  "if ((support) || (forceType)) {\n        if ((!forceType) || ((forceType) && (vi" \
  "deo.source[i].type.indexOf(forceType) >= 0))) {\n          if (foundPlayer === f" \
  "alse) { \n            foundPlayer = i;\n            if (!forceSupportCheck) {\n " \
  "             break;\n            }\n          }\n        }\n      }\n    }\n    " \
  "if (foundPlayer === false) {\n      // of all the streams given, none was suppor" \
  "ted (eg. no flash and HTML5 video). Display error\n      container.innerHTML = '" \
  "<strong>No support for any player found</strong>';\n    }\n    else {\n      // " \
  "we support this kind of video, so build it.\n      buildPlayer(video.source[foun" \
  "dPlayer], container, video.width, video.height, vtype);\n    }\n  }\n  \n  retur" \
  "n (mistvideo[streamname].embedded ? mistvideo[streamname].embedded.type : false)" \
  ";\n}\n";
unsigned int embed_js_len = 11007;
