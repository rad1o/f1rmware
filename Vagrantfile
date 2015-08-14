Vagrant.configure('2') do |config|
  config.vm.define :rad1o do |rad1o_config|
    rad1o_config.vm.box     = 'ubuntu/trusty64'
    rad1o_config.vm.hostname = 'rad1o'
    rad1o_config.vm.network :private_network, ip: '10.0.10.2'
    rad1o_config.vm.provider :virtualbox do |vb|
      vb.customize [
        'modifyvm', :id,
        '--name', 'rad1o vagrant vm'
      ]
    end

    rad1o_config.vm.provision 'shell', inline: 'apt-get install software-properties-common'
    rad1o_config.vm.provision 'shell', inline: 'add-apt-repository -y ppa:terry.guo/gcc-arm-embedded'
    rad1o_config.vm.provision 'shell', inline: 'echo "Package: gcc-arm-none-eabi\n Pin: release o=LP-PPA-terry.guo-gcc-arm-embedded\n Priority: 501" |sudo tee /etc/apt/preferences.d/pin-gcc-arm-embedded'
    rad1o_config.vm.provision 'shell', inline: 'apt-get update; apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi git cmake'
  end
end
