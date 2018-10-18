const net = require('net');
var readline = require('readline');
// node的控制台读入操作真是让人火大

//创建readline接口实例
var rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

var client = new net.Socket();
client.connect(7720, '127.0.0.1');

client.on('connect', () => {
    console.log('connected to server');
    let echo = () => {
        rl.question('Input: ', function(answer){
            // 不加close，则不会结束
            client.write(answer);
            if(answer === 'exit'){
                rl.close();
                client.destroy();
            } 
            else echo();
        });
    }
    echo();
});

client.on('data', (data) => {
    console.log('receive from server: ', data.toString());
});

client.on('close', (err) => {
    console.log('close');
    client = null;
    console.log(client);
});