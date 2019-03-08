const net = require('net');

// nodejs的socket server天生就是多线程的(多客户端同时连接)
// 用户逻辑只要管理好客户端的连接就可以了, 不管是用数组还是字典.
// 建立连接后处理客户端信息.
// 这里的socket参数就是客户端连接
var server = net.createServer();
server.on('connection', (socket) => {
    console.log('client connected');
    // socket 为客户端接入后创建的对象
    console.log('服务地址: ', socket.address()); // 输出服务监听的地址
    console.log('客户端地址: ', socket.remoteAddress + ':' + socket.remotePort) // 客户端地址
    socket.on('data', (data) => {
        if(data.toString() === 'exit') socket.destroy();
        console.log('receive from client: ', data.toString());
    });
    // 客户端正常关闭
    socket.on('close', (err) => {
        console.log('client disconnected');
    });
    // 客户端异常关闭
    socket.on('error', (err) => {
        console.log(err);
    });
});
server.on('end', () => {
    console.log('client disconnected');
});
server.on('error', (err) => {
    console.log(err);
});
server.listen(7720, '0.0.0.0', () => {
    console.log('server listening...');
});