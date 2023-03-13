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
                        SCANNERS = 'checkmarx, coverity'
			BANDIT_SOURCE_PATH   = '.'
                        SNYK_MANIFEST_FILE = ''

                        COVERITY_PROJECT_NAME = 'Software Vsync Modulation Sample'
                        COVERITY_PROJECT_VERSION = 'v1.0.0' // this is not the real version, just testing for report generation
                        COVERITY_BUILD_SCRIPT = 'make release'
                        COVERITY_CHECKER_CONFIG = '--concurrency --security --rule --enable-constraint-fpp --enable-fnptr --enable-virtual'
                        COVERITY_COMPILER = 'c++'
                        COVERITY_COMPILER_TYPE = 'gcc'
                    }
                    steps {
                        rbheStaticCodeScan()
                    }
                }
            }
        }
    }
}
