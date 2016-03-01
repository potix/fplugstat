
var fplugstatd = {
    select_view: function(body_id, menu_id) {
        $("#body-fplug-realtime").hide();
        $("#sidebar-fplug-realtime").css("color","black");
        $("#body-fplug-hourly").hide();
        $("#sidebar-fplug-hourly").css("color","black");
        $("#body-fplug-control").hide();
        $("#sidebar-fplug-control").css("color","black");
        $("#" + body_id).show();
        $("#" + menu_id).css("color","red");
    },
    current_device: null,
    realtime_interval: null,
    hourly_interval: null,
    realtime_vars: [],
    hourly_power_vars: [],
    hourly_other_vars: [],
}

$(document).ready(function(){
    $("#body-fplug-realtime").tabs({activate: function( event, ui ) {
        console.log("update chart")
        console.log(ui.newTab);
    }});
    $("#body-fplug-hourly").tabs({activate: function( event, ui ) {
        console.log("update chart")
        console.log(ui.newTab);
    }});
    $("#sidebar-fplug-device").change(function() {
        fplugstatd.current_device = $("#sidebar-fplug-device").val();
    });
    $("#sidebar-fplug-realtime").click(function(event) {
        fplugstatd.select_view("body-fplug-realtime", $(event.target).attr("id"));
        if (fplugstatd.hourly_interval != null) {
            clearInterval(fplugstatd.hourly_interval);
            fplugstatd.hourly_interval = null;
        }
        fplugstatd.realtime_interval = setInterval(function(){
            console.log("ajax realtime");
        }, 1000 * 5);
	return false;
    });
    $("#sidebar-fplug-hourly").click(function(event) {
        fplugstatd.select_view("body-fplug-hourly", $(event.target).attr("id"));
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
        fplugstatd.hourly_interval = setInterval(function(){
            console.log("ajax hourly");
        }, 1000 * 60);
	return false;
    });
    $("#sidebar-fplug-control").click(function(event) {
        fplugstatd.select_view("body-fplug-control", $(event.target).attr("id"));
        if (fplugstatd.hourly_interval != null) {
            clearInterval(fplugstatd.hourly_interval);
            fplugstatd.hourly_interval = null;
        }
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
	return false;
    });
    fplugstatd.select_view("body-fplug-realtime", "sidebar-fplug-realtime");
    fplugstatd.realtime_interval = setInterval(function(){
        console.log("ajax realtime");
    }, 1000 * 5);
    $.ajax({
        method: "GET",
        url: "/api/devicies",
        cache: false
    }).done(function(msg) {
        for (var i = 0; i < msg.length; i++) {
            $("#sidebar-fplug-device").append($("<option>").val(msg[i].address).text(msg[i].name));
        }
        fplugstatd.current_device = $("#sidebar-fplug-device").val();
    }) 
});



// あとはjavascriptかく

/*

// view切り替えイベント (デフォルトはリアルタイム)
                        <li class="menu-child" id="sidebar-fplug-realtime" > リアルタイム情報</li>
                        <li class="menu-child" id="sidebar-fplug-hourly"> 時間毎の情報 </li>
                        <li class="menu-child" id="sidebar-fplug-realtime"> 制御 </li>



// chart 表示 1
                       <div id="body-fplug-realtime-temperature-fragment">
                           <div id="body-fplug-realtime-temperature-chart">
                               1
                           </div>
                       </div>
                       <div id="body-fplug-realtime-humidity-fragment">
                           <div id="body-fplug-realtime-humidity-chart">
                               2
                           </div>
                       </div>
                       <div id="body-fplug-realtime-illuminance-fragment">
                           <div id="body-fplug-realtime-illuminance-chart">
                               3
                           </div>
                       </div>
                       <div id="body-fplug-realtime-watt-fragment">
                           <div id="body-fplug-realtime-watt-chart">
                               4
                           </div>
                       </div>

                        
// chart 表示 1
                       <div id="body-fplug-hourly-temperature-fragment">
                           <div id="body-fplug-hourly-temperature-chart">
                               1
                           </div>
                       </div>
                       <div id="body-fplug-hourly-humidity-fragment">
                           <div id="body-fplug-realtime-humidity-chart">
                               2
                           </div>
                       </div>
                       <div id="body-fplug-hourly-illuminance-fragment">
                           <div id="body-fplug-hourly-illuminance-chart">
                               3
                           </div>
                       </div>
                       <div id="body-fplug-hourly-watt-fragment">
                           <div id="body-fplug-hourly-watt-chart">
                               4
                           </div>
                       </div>
// 制御イベント
                    <div id="body-fplug-control">
                        <button type="button" id="body-fplug-control-reset">リセット</button><br /><br />
                        <button type="button" id="body-fplug-control-powerstart">電力取得開始</button><br /><br />
                        <button type="button" id="body-fplug-control-datetimesetting">時刻設定</button><br /><br />
                    </div>

*/
