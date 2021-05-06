

//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<html>

<head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Controlador de Riego | Ajustes</title>
</head>
<strong>Ajustes</strong>
<hr>
<a href="config.html" style="width:250px" class="btn btn--m btn--blue" >Configuraci&oacute;n de Red</a><br>
<a href="ntp.html"   style="width:250px"  class="btn btn--m btn--blue" >Configuraci&oacute;n NTP</a><br>
<a href="info.html"   style="width:250px"  class="btn btn--m btn--blue" >Informaci&oacute;n de Red</a><br>
<a href="javascript:ClientInit()" style="width:250px"  class="btn btn--m btn--blue">Conectar a la Red</a><br>

<script>
function ClientInit()
{
	setValues("/admin/clientinit");
}
window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				// Do something after load...
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

</script>
</html>
)=====";


void send_client_init_html()
{
	adminTimeOutCounter = ADMIN_TIMEOUT;
}

