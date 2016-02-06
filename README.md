Small XMPP client daemon for use in embedded systems ( OpenWRT etc. ).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

It depends on libstrophe ( http://strophe.im/libstrophe/ )

This daemon connects to XMPP server and do two simple tasks:

1. Wait for messages from server. Received messages saved as 
files into /var/spool/xmppcd/in directory.

2. Watch /var/spool/xmppd/out directory for new messages to send.

Message must be in the following format:

    ----- file start ------
    To: some_user@xmpp-server.com
    
    Message text here
    ------ file end -------

Message files must be _moved_ into /var/spool/xmppd/out, not created in it.

Simple shell script xmpp-send provided to send messages from command line:

    $ xmpp-send some_user@xmpp-server.com "message text"

Messages sent to server will be moved to /var/spool/xmppcd/sent directory.

Sent and received messages have numeric names, part before the dot is UNIX timestamp,
part after the dot is microseconds.

Don't forget to do

    $ chown -R user:group /var/spool/xmppcd

after install, using user and group you set in /etc/xmppcd.conf
