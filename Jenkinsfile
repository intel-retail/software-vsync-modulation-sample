pipeline {
    agent { label 'docker' }
    options {
        disableConcurrentBuilds()
        buildDiscarder(logRotator(daysToKeepStr: '30'))
    }
    stages {
        stage('Scans') {
            parallel {
                stage('Static code scan') {
                    environment {
                        PROJECT_NAME = 'software-vsync-modulation'
			SNYK_PROJECT_NAME = ''
                        SCANNERS = 'checkmarx'
			BANDIT_SOURCE_PATH   = '.'
                        SNYK_MANIFEST_FILE = ''
                    }
                    steps {
                        rbheStaticCodeScan()
                    }
                }
            }
        }
    }
}
