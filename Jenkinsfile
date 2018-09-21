pipeline {
    agent {
        dockerfile {
              filename 'Dockerfile.build'
              args '-v pip_cache:/var/pip_cache'
        }
    }
    stages {
        stage('Build') {
            steps {
                sh 'pwd'
                sh 'rm -Rf cbuild'
                sh 'mkdir cbuild'
                sh 'cd cbuild/ && cmake .. && make'
                sh 'rm -Rf env'
                sh 'python3 -m venv env'
                sh '''
                        . ./env/bin/activate && 
                        pip3 --cache-dir /var/pip_cache install pytest numpy &&
                        pip3 install -e . &&
                        sh 'GIT_SSH_COMMAND="ssh -i /var/jenkins_home/.ssh/jenkins_eqasm_assembler_deploy" git clone --branch v2.2.0 git@github.com:QE-Lab/eQASM_Assembler.git' &&
                        cd qisa-as &&
                        pip3 install .
                   '''
            }
        }
        stage('Test') {
            steps {
                sh '''
                        . ./env/bin/activate &&
                        pytest -k-test_QISA_assembler_present
                   '''
            }
        }

    }
}
