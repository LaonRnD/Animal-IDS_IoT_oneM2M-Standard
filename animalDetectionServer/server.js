///////////////Parameters/////////////////

//////////////////////////////////////////
var shortid = require('shortid');
var http = require('http');
var express = require('express');
var request = require('request')
var indexRouter = require('./routes/index');
const devicesRouter = require("./routes/devices");
const usersRouter = require("./routes/users");
const crypto = require('crypto');
var flash = require('connect-flash');
//var qs = require('qs');
var fs = require('fs');
var url = require('url');
var util = require('util');
var app = express();
_ = require('underscore');
var fs = require('fs');
const moment = require("moment");//시간 값 가져오기
const date = moment();
var path = require('path');
const session = require('express-session');
const FileStore = require('session-file-store')(session); // 세션을 파일에 저장
const cookieParser = require('cookie-parser');
const cors = require('cors');

//const multer = require('multer');
const fileUpload = require('express-fileupload');

const MIME_TYPE_MAP = {
    "image/png": "png",
    "image/jpeg": "jpeg",
    "image/jpg": "jpg",
    "application/pdf": "pdf",
};
var noti_flag1 = 1;//1 : no detect, 0 : detect
var amb_temp1 = 0;
var obj_temp1 = 0;
var diff_temp1 = 0;
var noti_flag2 = 1;
var amb_temp2 = 0;
var obj_temp2 = 0;
var diff_temp2 = 0;
var detect_flag = 0;
var noti_cam = 0;

global.amb_temp1 = amb_temp1;
global.obj_temp1 = obj_temp1;
global.diff_temp1 = diff_temp1;
global.noti_flag1 = noti_flag1;
global.amb_temp2 = amb_temp2;
global.obj_temp2 = obj_temp2;
global.diff_temp2 = diff_temp2;
global.noti_flag2 = noti_flag2;
global.detect_flag = detect_flag;
global.noti_cam = noti_cam;

var mqtt_sub_client = null;
global.mqtt_sub_client = mqtt_sub_client;

// mysql 연결 모듈 불러오기
var db = require('./db');
db.connect(function(err) {
  if (err) {
	console.log('Unable to connect to MySQL.')
	process.exit(1)
  }
  else console.log('Connected!');
});

app.use(cors());

// set the view engine to ejs
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views/pages'));
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.urlencoded({ extended: false }));;
app.use(cookieParser());
app.use(express.json());
app.use(fileUpload({
    useTempFiles : true,
    tempFileDir : '/tmp/',
    debug: false
}));
app.use(cookieParser('abcdlaon'));
// 세션 (미들웨어)
//rolling을 true로 셋팅
//(saveUnintialized는 기본 false인데 true로 셋팅해야 rolling이 유효함, https://github.com/expressjs/session#rolling) 
app.use(session({
    key: 'sid',
	secret: 'abcdlaon', // 데이터를 암호화 하기 위해 필요한 옵션
    resave: false, // 요청이 왔을때 세션을 수정하지 않더라도 다시 저장소에 저장되도록
    saveUninitialized: true, // 세션이 필요하면 세션을 실행시칸다(서버에 부담을 줄이기 위해)
    store : new FileStore(), // 세션이 데이터를 저장하는 곳
	cookie: {
		maxAge: 1000 * 60 * 60 // 쿠키 유효기간 1시간 600000
	},
	rolling : true //세션이 최대 10분이 유지되며 새로고침이나 페이지이동시 세션만료가 갱신되는 설정이다.
}));
app.use(flash());
//응답 객체에 플래시 메시지의 로컬 flashMessages로 할당 후 이후 뷰에서 등록한 키로 접근 가능
app.use((req, res, next)=>{
	res.locals.flashMessages = req.flash();
	next();
});

app.use('/', indexRouter);
app.use("/devices", devicesRouter);
app.use("/users", usersRouter );

// 회원가입
app.get('/register', (req, res) => {
	const sess = req.session;    
    if(sess.username){
        res.render('login',{
            is_logined : true,
            id : sess.username
        });
    }else{
		sess.is_logined = false;
		sess.username = null;
        res.render('register',{
            is_logined : false
        });
    }
});
app.post('/register', (req, res) => {
  const body = req.body;
  const id = body.id;
  const nick = body.nick;
  const email = body.email;
  const pw = body.pw;
  const photo_path = "/public/uploads/" + id;
  console.log('path',__dirname);
  const ae_name = 'ae_' + id ;
  //password 암호화
  const salt = Math.round((new Date().valueOf() * Math.random())) + "";
  const hashPw = crypto.createHash("sha512").update(pw + salt).digest("hex");
  const create_at = date.format("YYYY-MM-DD");
   console.log('try to register');
   db.get().query('select * from users where id=?',[id],(err,data)=>{
  if(!err) {
		if(data.length == 0){		
			
			fs.exists(path.join(__dirname, photo_path), exists => {
			  console.log(exists ? "The directory already exists" 
								: "Not found!");
			  if(!exists) {
				  fs.mkdirSync(path.join(__dirname, photo_path), true);				  
				  console.log('mkdir ok');		
			  }
			});					
            	
			db.get().query('insert into users(id, nick, email,password,salt,photo_path,ae_name,create_at) values(?,?,?,?,?,?,?,?)',[id,nick,email,hashPw,salt,photo_path,ae_name,create_at]);
			console.log('회원가입 성공');
			res.redirect('/users/login');
		}else{
			console.log('회원가입 실패');       
			res.redirect('/users/login');
		   
		}
		
	 
  } else {
	   console.log('등록 실패');       
		res.redirect('/users/login');
  }
 });
});

// about page
app.get('/about', (req, res) => {
  res.render('pages/about',{
				is_logined : false
			});	
});
app.get('/upload', (req,res)=> {
	const sess = req.session;
	console.log(sess);
    if(sess.username){
        res.render('upload',{
            is_logined : true,
            id: sess.username
        });
    }else{
        res.render('login',{
            is_logined : false
        });
    }	
});
app.post('/upload', function(req, res) {
	let sampleFile;
	let uploadPath;
	let user = req.session;
	let id = user.username;	
	console.log('user_id',user);
	if (!req.files || Object.keys(req.files).length === 0) {
	  return res.status(400).send('No files were uploaded.');
	}
	let date = new Date();
	let formatatedDate = (addZero(date.getFullYear())) + "-" + (addZero(date.getMonth() + 1)) + "-" + date.getDate() + "_" + addZero(date.getHours()) + addZero(date.getMinutes()) + addZero(date.getSeconds());
	console.log(`data:${formatatedDate}`);
	// The name of the input field (i.e. "sampleFile") is used to retrieve the uploaded file
	sampleFile = req.files.profile_pic;
	uploadPath = path.join(__dirname, 'public') + '\\uploads\\' + formatatedDate+".jpg";
	console.log('photo path:',uploadPath);
	// Use the mv() method to place the file somewhere on your server
	sampleFile.mv(uploadPath, function(err) {
	  if (err) {
		return res.status(500).send(err);
	  }
	  else {
		noti_cam = 1;
		console.log('noti_cam:',noti_cam);
		//res.send('ok');
		res.render('/devices/device_con');
	  }
	});
  });
  app.post('/upload/:name', function(req, res) {
	let user_path = req.params.name;
	console.log('path:',user_path);
	let sampleFile;
	let uploadPath;
	if (!req.files || Object.keys(req.files).length === 0) {
	  return res.status(400).send('No files were uploaded.');
	}
	let date = new Date();
	let formatatedDate = (addZero(date.getFullYear())) + "-" + (addZero(date.getMonth() + 1)) + "-" + date.getDate() + "_" + addZero(date.getHours()) + addZero(date.getMinutes()) + addZero(date.getSeconds());
	console.log(`data:${formatatedDate}`);
	// The name of the input field (i.e. "sampleFile") is used to retrieve the uploaded file
	var file_name = formatatedDate+".jpg";
	sampleFile = req.files.profile_pic;
	uploadPath = path.join(__dirname, 'public','uploads',user_path,file_name);//+ user_path +'/' + formatatedDate+".jpg";
	console.log('photo path:',uploadPath);
	// Use the mv() method to place the file somewhere on your server
	sampleFile.mv(uploadPath, function(err) {
	  if (err) {
		return res.status(500).send(err);
	  }
	  else {
		noti_cam = 1;
		console.log('noti_cam:',noti_cam);
		res.send('ok');
	  }
	});
  });

//var data11 = new Object();
app.get('/in_list',async (req,res,next)=>{
	var sess = req.session;	
	var file_list =[];	
	const user_id = sess.username;
	console.log('user_id :' + user_id); 
    var folder = path.join(__dirname, 'public','uploads',user_id);
    console.log('uploadPath :' + folder); 	
	 	
	if(user_id) {	
		fs.readdir(folder, function(error, filelist){
			console.log(filelist); 
			file_list = filelist;
			file_list.forEach((file, index) => {
			  file_list[index] = file;
			  //console.log('f',file);			  
			})
			console.log("filelist");
			res.render('image_list',{is_logined : true,
				id : user_id,
				data :file_list 
			});

		});	
	}else {
		console.log('login again'); 		
		res.send('in_list fail');	
	}

});
app.get('/in_list/:name',async (req,res,next)=>{	
	var sess = req.session;
	var id = req.params.name;
	var file_list =[];	
	console.log('id :' + id);   
	//const title = "Sensor View";
    //console.log('folder',folder);
	//const sess = req.session;
	//console.log('image_details 2:');
	//var id = sess.username;//req.query.name;
	const user_id = id;
	console.log('user_id :' + user_id); 
    var folder = path.join(__dirname, 'public','uploads',id);
    console.log('uploadPath :' + folder); 	
	 	
	if(user_id) {	
		fs.readdir(folder, function(error, filelist){
			console.log(filelist); 
			file_list = filelist;
			file_list.forEach((file, index) => {
			  file_list[index] = file;
			  //console.log('f',file);			  
			})
			console.log("filelist");
			res.render('image_list',{is_logined : true,
				id : user_id,
				data :file_list 
			});

		});	
	}else {
		console.log('login again'); 		
		res.send('in_list fail');	
	}

});

app.get('/image_details', (req, res,next) => {
	const sess = req.session;
	console.log('image_details 2:');
	var id = sess.username;//req.query.name;
	const fname = req.query.file_name;	
	console.log('file :' + fname);
	console.log('id :' + id);
	
	//var dir = process.cwd();
	//console.log('file dir :' + dir);	
	console.log('base folder :' + __dirname);	
	const xt_dir = '/public/uploads/'+ id +'/' + fname;  
	const dir_path= path.join(__dirname,xt_dir);
	//ejs 파일은 public 폴더 기준으로 접근하기 때문에 현재 폴더 기본이 use(express.static(path.join(__dirname, 'public'))) 로 세팅되어 있음
	//그래서 public 폴더 경로는 삭제함
	const dir_ = './uploads/'+ id +'/' + fname;   
	
	if(sess.username){
		    if(fs.existsSync(dir_path)) {	
				console.log('file dir :' + xt_dir);
				res.render('image_details',{id:id,
					is_logined : true,
					file_path: dir_// dir_path 사용하지 않음. u
				});
			} else {
				res.end('file is not exists',xt_dir);
			}						

	} else res.redirect('/login');
		
			
			

});

//app.listen(3000);
//console.log('Server is listening on port 3000');
app.set("port", "3000");
var server = http.createServer(app);
var io = require("socket.io")(server);
io.on('connection',(socket)=>{
	socket.on("login user", function (data, cb) {
		if (loginCheck(data)) {
			onlineUsers[data.id] = {roomId: 1, socketId: socket.id};
			socket.join('room' + data.roomId);
			cb({result: true, data: "로그인에 성공하였습니다."});
		} else {
			cb({result: false, data: "등록된 회원이 없습니다. 회원가입을 진행해 주세요."});
			return false;
		  }
	});		

});
//웹서버 구동 구동
server.listen(3000, (err) => {
	if (err) {
		return console.log(err);
	} else {
		console.log("Server is listening on port 3000");
	
	}
});

function addZero(number){
	if (number <= 9)
		return "0" + number;
	else
		return number;
}
//socket 이벤트 처리
function loginCheck(data) {
	if (users.hasOwnProperty(data.id) && users[data.id].pw === data.pw) {
		return true;
	} else {
		return false;
	}
}
function joinCheck(data) {
	if (users.hasOwnProperty(data.id)) {
		return true;
	} else {
		return false;
	}
}
function getUserBySocketId(id) {
	return Object.keys(onlineUsers).find(key => onlineUsers[key].socketId === id);
}

