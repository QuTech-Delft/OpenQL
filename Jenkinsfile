pipeline {
    agent {
        dockerfile {
              filename 'Dockerfile.build'
	      args '-u 0'
        }
    }
    stages {
        stage('Build') {
            steps {
                sh 'pwd'
                sh 'mkdir cbuild'
                sh 'cd cbuild/ && cmake .. && make'
                sh 'pip3 install pytest numpy'
                sh 'pip3 install -e .'
            }
        }
        stage('Test') {
            steps {
                sh 'pytest -k-test_QISA_assembler_present'
            }
        }

    }
}
