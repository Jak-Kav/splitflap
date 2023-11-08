define(["exports"],(function(e){"use strict";const o=function(e,o){if(!e)throw new Error(o)},t=/^([0-9]{1,3}\.){3}[0-9]{1,3}$/,n=function(e){if(e.match(t))return[e];const o=e.split("."),n=[];for(let e=0;e<o.length;e++)n.push(o.slice(e).join("."));return n},i=function(e){const o=e.split("/"),t=[];for(let e=0;e<o.length;e++){const n=o.slice(0,o.length-e).join("/");""!==n&&t.push(n),t.push(n+"/")}return t},a=["=",";"],s=function(e,t,n=!1){o("string"==typeof e,`${t} must be a string, but was ${typeof e}`),o(n||e.length>0,`${t} must not be empty`),o(!function(e){if(null==e)return!1;for(const o of a)if(-1!==e.indexOf(o))return!0;return!1}(e),`${t} contains illegal characters`)},r=e=>s(e,"Cookie name",!1),c=e=>{return t=e,n="Cookie max age",void o(!isNaN(Number(t)),`${n} must be numeric, but was ${t}`);var t,n},u=function(e,o,t={}){var n,i;r(e),(e=>{s(e,"Cookie value",!0)})(o),t.maxAge&&c(t.maxAge),t.domain&&(n=t.domain,s(n,"Cookie domain",!1)),t.path&&(i=t.path,s(i,"Cookie path",!1));const a=[`${e}=${o}`];t.maxAge&&a.push(`max-age=${t.maxAge}`),t.domain&&a.push(`domain=${t.domain}`),t.path&&a.push(`path=${t.path}`),t.samesite&&a.push(`samesite=${t.samesite}`),a.push("secure"),document.cookie=a.join("; ")},m={create(e,t,n,i,a="/",s="lax"){let r;null!=n&&(o("number"==typeof n,"Cookie max age days must be a number"),r=24*n*60*60),u(e,t,{maxAge:r,path:a,domain:i,samesite:s})},read(e){r(e);const o=[];for(const t of document.cookie.split(";")){const n=t.split("=",1)[0];n.trim()===e&&o.push(t.substring(n.length+1,t.length).trim())}let t=!1;const n=[];for(const e of o)""===e?t=!0:n.push(e);return 1===n.length?n[0]:n.length>1?(m.delete(e),null):t?"":null},delete(e){const o=[null,...n(window.location.hostname)],t=[null,...i(window.location.pathname)];for(const n of o)for(const o of t)u(e,"",{maxAge:-1,domain:n,path:o,samesite:"lax"})},are_enabled:()=>navigator.cookieEnabled?navigator.cookieEnabled:(document.cookie="this_is_a_test_cookie",-1!==document.cookie.indexOf("this_is_a_test_cookie"))};e.Cookies=m,e.assertDropboxDomain=function(e){const o=".dropbox.com",t=document.createElement("a");t.href=e;const n=t.hostname||window.location.hostname;if(-1===n.indexOf(o,n.length-12))throw new Error("Cannot send the CSRF token to "+n)},e.readCsrfToken=function(){return m.read("__Host-js_csrf")}}));
//# sourceMappingURL=c_csrf.js-vfllJuysR.map
